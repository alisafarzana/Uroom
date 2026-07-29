# URoom — Streamlit Frontend + C++ Backend

## Included files

- `app.py` — Streamlit user interface
- `backend.cpp` — C++ filtering, availability, distance lookup, booking and Min Heap ranking
- `bookings.csv` — booking records
- `dataset_graph_final.csv` — dataset of baseline and optimized algorithm test
- `requirements.txt` — Python packages
- `run_app.bat` — compiles C++ and starts Streamlit
- `build_backend.bat` — compiles only the C++ file

## How both files work together

Streamlit collects user input. Python starts `backend.exe` using `subprocess`. The C++ program performs the algorithm and prints ranked room results. Python reads that output and displays it as a table, schedule, booking form and charts.

## Run on Windows

Install Python and a C++ compiler that provides `g++`, such as MinGW-w64 or MSYS2. Confirm:

```powershell
g++ --version
```

Then double-click `run_app.bat`, or run manually:

```powershell
g++ backend.cpp -std=c++17 -O2 -o backend.exe
python -m pip install -r requirements.txt
python -m streamlit run app.py
```

The website normally opens at:

```text
http://localhost:8501
```

## Booking rules

- Each booking is two hours.
- IRC offers booking slots from 10 AM to 10 PM
- Preferred time, starting location and preferred location are optional.
- Results appear in one ranked list.
