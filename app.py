from __future__ import annotations

from datetime import date
from pathlib import Path
import os
import subprocess
import pandas as pd
import streamlit as st

st.set_page_config(page_title="URoom", page_icon="🏫", layout="wide")
st.markdown(
    """
    <style>
    .block-container{max-width:1450px;padding-top:1.3rem;padding-bottom:4rem}
    .hero{font-size:clamp(2.2rem,4vw,4.1rem);font-weight:800;line-height:1.03;letter-spacing:-.04em;margin:.4rem 0 .7rem}
    .muted{color:#647784;line-height:1.65}.eyebrow{color:#08726f;font-size:.75rem;font-weight:800;letter-spacing:.12em;text-transform:uppercase}
    .section{font-size:2rem;font-weight:800;letter-spacing:-.03em;margin:.25rem 0 1rem}
    .card{padding:1.2rem;border-radius:18px;color:white;min-height:160px}.red{background:linear-gradient(135deg,#7c3f3f,#b75c52)}
    .green{background:linear-gradient(135deg,#063b5c,#0d8a86)}.grey{background:linear-gradient(135deg,#4f5f6b,#748995)}
    .card small{font-weight:800;letter-spacing:.08em;text-transform:uppercase;opacity:.8}.card b{display:block;font-size:2.35rem;margin:.75rem 0}
    </style>
    """,
    unsafe_allow_html=True,
)

ROOT = Path(__file__).resolve().parent

BACKEND = ROOT / (
    "backend.exe"
    if os.name == "nt"
    else "backend"
)

SOURCE = ROOT / "backend.cpp"

COMPARISON_DATA = (
    ROOT
    / "uroom_algorithm_comparison_dataset.csv"
)

SLOTS = [
    "Any available time",
    "08:00-10:00",
    "10:00-12:00",
    "12:00-14:00",
    "14:00-16:00",
    "16:00-18:00",
    "18:00-20:00",
    "20:00-22:00",
]
LOCATIONS = [
    "Not specified",
    "Block B",
    "Chancellor Complex",
    "Information Resource Centre",
    "Nadi@UTP",
    "Pocket D",
    "Village 1",
    "Village 5",
]
PREFERRED = [
    "Any location",
    "Block B",
    "Chancellor Complex",
    "Information Resource Centre",
    "Nadi@UTP",
    "Village 5",
]


def compile_backend() -> tuple[bool, str]:
    if BACKEND.exists():
        return True, "C++ backend ready."
    try:
        completed = subprocess.run(
            ["g++", str(SOURCE), "-std=c++17", "-O2", "-o", str(BACKEND)],
            cwd=ROOT,
            capture_output=True,
            text=True,
            check=True,
        )
        return True, completed.stdout.strip() or "C++ backend compiled."
    except FileNotFoundError:
        return False, "g++ was not found. Install MinGW-w64 or MSYS2, then run run_app.bat."
    except subprocess.CalledProcessError as error:
        return False, error.stderr.strip() or "C++ compilation failed."


def call_backend(arguments: list[str]) -> list[str]:
    if not BACKEND.exists():
        ok, message = compile_backend()
        if not ok:
            raise RuntimeError(message)
    try:
        completed = subprocess.run(
            [str(BACKEND), *arguments],
            cwd=ROOT,
            capture_output=True,
            text=True,
            check=True,
        )
    except subprocess.CalledProcessError as error:
        raise RuntimeError(
            error.stderr.strip() or error.stdout.strip() or "C++ backend error"
        ) from error
    return [line.strip() for line in completed.stdout.splitlines() if line.strip()]


def search_rooms(pax: int, day: str, slot: str, start: str, preferred: str):
    lines = call_backend(
        [
            "search",
            str(pax),
            day,
            "" if slot == "Any available time" else slot,
            start,
            preferred,
        ]
    )
    metadata = {"candidate": 0, "total": 0}
    rows = []
    for line in lines:
        parts = line.split("|")
        if parts[0] == "META":
            metadata = {"candidate": int(parts[1]), "total": int(parts[2])}
        elif parts[0] == "RESULT":
            rows.append(
                {
                    "Rank": int(parts[1]),
                    "Room ID": parts[2],
                    "Room": parts[3],
                    "Location": parts[4],
                    "Capacity": int(parts[5]),
                    "Extra seats": int(parts[6]),
                    "Distance": "Not considered"
                    if parts[7] == "-1"
                    else f"{parts[7]} m ({parts[8]} min)",
                    "Available time": parts[9],
                    "Score": float(parts[10]),
                    "_slots": parts[11].split(",") if parts[11] else [],
                }
            )
    return metadata, rows


def get_schedule(room_id: str, day: str):
    output = []
    for line in call_backend(["schedule", room_id, day]):
        parts = line.split("|")
        if parts[0] == "SLOT":
            output.append({"slot": parts[1], "status": parts[2], "label": parts[3]})
    return output


def book_room(room_id: str, day: str, slot: str) -> str:
    lines = call_backend(["book", room_id, day, slot])
    if lines and lines[0].startswith("OK|"):
        return lines[0].split("|", 1)[1]
    raise RuntimeError("Booking was not confirmed.")


def get_benchmark() -> pd.DataFrame:
    if not COMPARISON_DATA.exists():
        raise RuntimeError(
            "The comparison dataset was not found: "
            f"{COMPARISON_DATA.name}"
        )

    rows = []

    lines = call_backend(
        [
            "benchmark",
            str(COMPARISON_DATA),
        ]
    )

    for line in lines:
        parts = line.split("|")

        if parts[0] == "BENCH" and len(parts) >= 10:
            rows.append(
                {
                    "Dataset size": int(parts[1]),

                    "Baseline operations":
                        float(parts[2]),

                    "Optimized operations":
                        float(parts[3]),

                    "Baseline time":
                        float(parts[4]),

                    "Optimized time":
                        float(parts[5]),

                    "Baseline memory":
                        float(parts[6]),

                    "Optimized memory":
                        float(parts[7]),

                    "Baseline records examined":
                        int(parts[8]),

                    "Optimized records examined":
                        int(parts[9]),
                }
            )

    if not rows:
        raise RuntimeError(
            "The C++ backend returned no valid "
            "comparison data."
        )

    return (
        pd.DataFrame(rows)
        .sort_values("Dataset size")
        .set_index("Dataset size")
    )


for key, default in {
    "query": None,
    "results": [],
    "meta": {},
    "show_schedule": False,
}.items():
    if key not in st.session_state:
        st.session_state[key] = default

st.markdown(
    '<b style="font-size:1.4rem">🏫 URoom</b>',
    unsafe_allow_html=True,
)
st.markdown(
    '<div style="height:24px"></div>'
    '<div class="eyebrow">UTP student facility prototype</div>'
    '<div class="hero">'
    'Find available study rooms based on your booking preferences.'
    '</div>',
    unsafe_allow_html=True,
)

ready, _ = compile_backend()

st.markdown(
    '<div style="height:24px"></div><div class="eyebrow">Search preferences</div>'
    '<div class="section">Find your room</div>',
    unsafe_allow_html=True,
)
with st.form("search"):
    c1, c2, c3, c4, c5 = st.columns(5)
    with c1:
        pax = st.number_input("Number of pax *", 1, 100, 20)
    with c2:
        day = st.date_input("Booking date *", date.today(), min_value=date.today())
    with c3:
        slot = st.selectbox("Preferred time slot", SLOTS)
    with c4:
        start = st.selectbox("Starting location", LOCATIONS)
    with c5:
        preferred = st.selectbox("Preferred location", PREFERRED)
    submitted = st.form_submit_button(
        "Find rooms", type="primary", use_container_width=True, disabled=not ready
    )

if submitted:
    start_value = "" if start == "Not specified" else start
    preferred_value = "" if preferred == "Any location" else preferred
    try:
        metadata, rows = search_rooms(
            int(pax), day.isoformat(), slot, start_value, preferred_value
        )
        st.session_state.query = {
            "pax": int(pax),
            "day": day.isoformat(),
            "slot": slot,
            "start": start_value,
            "preferred": preferred_value,
        }
        st.session_state.meta = metadata
        st.session_state.results = rows
        st.session_state.show_schedule = False
    except RuntimeError as error:
        st.error(str(error))

if st.session_state.query:
    query = st.session_state.query
    rows = st.session_state.results
    st.divider()
    st.markdown(
        '<div class="eyebrow">Available options</div><div class="section">Suitable rooms</div>',
        unsafe_allow_html=True,
    )
    st.caption(
        f"{query['pax']} pax · {query['day']} · {query['slot']} · "
        f"Starting: {query['start'] or 'Not specified'} · "
        f"Preferred: {query['preferred'] or 'Any location'}"
    )
    if not rows:
        st.warning(
            "No suitable room found. Try Any available time, remove preferred location, or reduce pax."
        )
    else:
    # Room ID is needed internally.
    # Distance and Score are used for ranking,
    # but they are hidden from the displayed table.
        hidden_columns = {
            "Room ID",
            "Distance",
            "Score",
        }

        display = [
            {
                key: value
                for key, value in row.items()
                if not key.startswith("_")
                and key not in hidden_columns
            }
            for row in rows
        ]


        table_event = st.dataframe(
            pd.DataFrame(display),
            use_container_width=True,
            hide_index=True,
            on_select="rerun",
            selection_mode="single-row",
            key="available_rooms_table",
        )

        selected_rows = table_event.selection.rows

        if not selected_rows:
            st.info(
                "Select a room by clicking its row in the table."
            )

        else:
            selected_index = selected_rows[0]

            if selected_index >= len(rows):
                st.warning(
                    "Please select the room again."
                )

            else:
                selected_room = rows[selected_index]

                room_id = selected_room["Room ID"]

                chosen = (
                    f"{selected_room['Rank']}. "
                    f"{selected_room['Room']} — "
                    f"{selected_room['Location']}"
                )

                st.success(
                    f"Selected room: {chosen}"
                )

                left, right = st.columns(2)

                with left:
                    if query["slot"] != "Any available time":
                        if st.button(
                            "Book Room",
                            type="primary",
                            use_container_width=True,
                        ):
                            try:
                                st.success(
                                    book_room(
                                        room_id,
                                        query["day"],
                                        query["slot"],
                                    )
                                )

                                (
                                    st.session_state.meta,
                                    st.session_state.results,
                                ) = search_rooms(
                                    query["pax"],
                                    query["day"],
                                    query["slot"],
                                    query["start"],
                                    query["preferred"],
                                )

                                st.rerun()

                            except RuntimeError as error:
                                st.error(str(error))

                    else:
                        st.info(
                            "Use View Schedule to choose a two-hour slot."
                        )

                with right:
                    if st.button(
                        "View Schedule",
                        use_container_width=True,
                    ):
                        st.session_state.show_schedule = (
                            not st.session_state.show_schedule
                        )

                if st.session_state.show_schedule:
                    st.subheader(
                        f"Schedule — {chosen}"
                    )

                    try:
                        for item in get_schedule(
                            room_id,
                            query["day"],
                        ):
                            col1, col2, col3 = st.columns(
                                [2, 1, 1.2]
                            )

                            with col1:
                                st.write(item["label"])

                            with col2:
                                if item["status"] == "Available":
                                    st.success(
                                        item["status"]
                                    )
                                else:
                                    st.error(
                                        item["status"]
                                    )

                            with col3:
                                if item["status"] == "Available":
                                    if st.button(
                                        "Book slot",
                                        key=(
                                            f"book_{room_id}_"
                                            f"{item['slot']}"
                                        ),
                                        use_container_width=True,
                                    ):
                                        try:
                                            st.success(
                                                book_room(
                                                    room_id,
                                                    query["day"],
                                                    item["slot"],
                                                )
                                            )

                                            st.rerun()

                                        except RuntimeError as error:
                                            st.error(str(error))

                                else:
                                    st.button(
                                        "Unavailable",
                                        key=(
                                            f"unavailable_{room_id}_"
                                            f"{item['slot']}"
                                        ),
                                        disabled=True,
                                        use_container_width=True,
                                    )

                    except RuntimeError as error:
                        st.error(str(error))

st.divider()
st.markdown(
    '<div class="eyebrow">Algorithm evaluation</div><div class="section">Improvement dashboard</div>',
    unsafe_allow_html=True,
)
m1, m2, m3 = st.columns(3)
with m1:
    st.markdown(
        '<div class="card red"><small>Baseline query</small><b>O(n)</b>Linear search checks all room records.</div>',
        unsafe_allow_html=True,
    )
with m2:
    st.markdown(
        '<div class="card green"><small>Optimized query</small><b>O(k + r log k)</b>Hash Map lookup narrows candidates before Min Heap ranking.</div>',
        unsafe_allow_html=True,
    )
with m3:
    st.markdown(
        '<div class="card grey"><small>Space complexity</small><b>O(n) vs O(n)</b>Optimized storage has a larger constant for indexes.</div>',
        unsafe_allow_html=True,
    )

if ready:
    try:
        benchmark = get_benchmark()

        chart1, chart2 = st.columns(2)

        with chart1:
            st.subheader("Estimated operations comparison")

            st.line_chart(
                benchmark[
                    [
                        "Baseline operations",
                        "Optimized operations",
                    ]
                ],
                use_container_width=True,
            )

        with chart2:
            st.subheader("Execution time comparison")

            st.line_chart(
                benchmark[
                    [
                        "Baseline time",
                        "Optimized time",
                    ]
                ],
                use_container_width=True,
            )

        chart3, chart4 = st.columns(2)

        with chart3:
            st.subheader("Memory usage comparison")

            st.bar_chart(
                benchmark[
                    [
                        "Baseline memory",
                        "Optimized memory",
                    ]
                ],
                use_container_width=True,
            )

        with chart4:
            st.subheader("Records examined comparison")

            st.bar_chart(
                benchmark[
                    [
                        "Baseline records examined",
                        "Optimized records examined",
                    ]
                ],
                use_container_width=True,
            )

    except RuntimeError as error:
        st.error(str(error))

    except KeyError as error:
        st.error(
            f"Dashboard column is missing: {error}"
        )
