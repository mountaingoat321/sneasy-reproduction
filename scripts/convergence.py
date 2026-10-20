"""Deterministic temporal and spatial refinement of the complete reference run."""
import csv
from pathlib import Path
import re
import subprocess
import numpy as np

ROOT = Path(__file__).resolve().parents[1]


def run(dt, cells):
    name = 'convergence_dt%s_n%d' % (str(dt).replace('.', '_'), cells)
    text = subprocess.check_output(['./build/model', 'reference', str(dt), str(cells), name],
                                   cwd=str(ROOT), universal_newlines=True)
    x = np.genfromtxt(ROOT/'results'/('%s.csv' % name), delimiter=',', names=True)
    cost = float(re.search(r'cost_USD=([\deE+.-]+)', text).group(1))
    result = [dt, cells, x['temperature_C'][250], x['sea_level_m'][250], cost,
              x['co2_ppm'][250], x['ohc_YJ'][250]]
    (ROOT/'results'/('%s.csv' % name)).unlink()
    return result


def main():
    cases = [(1., 80), (.5, 80), (.25, 80), (.125, 80), (.5, 20), (.5, 40), (.5, 160)]
    with (ROOT/'results/convergence.csv').open('w') as f:
        writer = csv.writer(f)
        writer.writerow(['dt_years', 'ocean_cells', 'temperature_2100_C', 'sl_2100_m',
                         'cost_usd2010', 'co2_2100_ppm', 'ohc_2100_YJ'])
        rows = [run(*case) for case in cases]
        writer.writerows(rows)
    # Absolute engineering tolerances, established independently of Figure 1 values.
    time_error = np.abs(np.array(rows[1][2:])-np.array(rows[3][2:]))
    space_error = np.abs(np.array(rows[5][2:])-np.array(rows[6][2:]))
    assert time_error[0] < .01 and time_error[1] < .01, time_error
    assert space_error[0] < .02 and space_error[1] < .02, space_error
    assert time_error[2]/rows[3][4] < .01, time_error
    assert space_error[2]/rows[6][4] < .01, space_error
    print('Time/grid refinement passed: <0.02 C, <0.02 m, <1% cost.')


if __name__ == '__main__':
    main()
