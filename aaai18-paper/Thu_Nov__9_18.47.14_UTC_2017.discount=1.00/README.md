#### Data for AAAI-18 paper: Planning with Pixels in (Almost) Real Time

Raw-data files:
* `raw_data_10.txt`
* `raw_data_11.txt`
* `raw_data_11n.txt`

Data files:
* `data10.csv`
* `data11.csv`
* `data11n.csv`

Python scripts:
* `extract_data.py`: creates .csv files from raw data. Usage: 
`python extract_data.py < raw_data_10.txt > data10.csv`
`python extract_data.py < raw_data_11.txt > data11.csv`
`python extract_data.py < raw_data_11n.txt > data11n.csv`

R scripts:
* `make_data.R`: reads .csv files, summarizes data, and create data frames.
* `make_plots.R`: calls `make_data.R` and then generate plots.
* `make_tables.R`: calls `make_data.R` and then generate tables.

Tables:
* `table1-011-15-onesec.txt`
* `table1-110-15-halfsec.txt`
* `table2a.txt`
* `table2b.txt`

Plots:
* `010-15-halfsec-bars.pdf`
* `010-15-halfsec.pdf`
* `011-15-halfsec-bars.pdf`
* `011-15-halfsec.pdf`
* `110-15-halfsec-bars.pdf`
* `110-15-halfsec.pdf`

