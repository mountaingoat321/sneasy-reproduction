"""Optional network verification of the fixed NOAA numerical snapshot."""
import csv
import hashlib
from pathlib import Path
from urllib.request import urlopen

root = Path(__file__).resolve().parents[1]
url = 'https://gml.noaa.gov/webdata/ccgg/trends/co2/co2_annmean_mlo.txt'
raw = urlopen(url, timeout=30).read()
upstream = {}
for line in raw.decode().splitlines():
    if line.strip() and not line.lstrip().startswith('#'):
        fields = line.split()
        upstream[int(fields[0])] = float(fields[1])
with (root/'data/co2_annual.csv').open() as f:
    for row in csv.DictReader(f):
        year = int(row['year'])
        assert abs(upstream[year]-float(row['co2_ppm'])) < 1e-8, ('Upstream revision', year)
print('Numerical snapshot agrees with current NOAA data.')
print('Current upstream byte SHA256:', hashlib.sha256(raw).hexdigest())
