"""Build a conservative four-box ocean from published impulse-response modes.

No model implementation or prepared data bundle is an input to this calculation.
The fast (1.271 y) mode is replaced by instantaneous air--sea equilibration.
"""
import csv
import json
from pathlib import Path
import numpy as np
from scipy.linalg import eigh, expm
from scipy.optimize import brentq

ROOT = Path(__file__).resolve().parents[1]


def pco2(d):
    t = 18.0
    return np.exp(-40.5797 + .074487*t - .000283*t*t + 63.69519*d
                  - 32.04292*d*d + 5.761687*d*d*d - .010416*t*d)


def main():
    d0 = brentq(lambda d: pco2(d)-278., 1.8, 2.3)
    dp = 278.*(63.69519 - 2*32.04292*d0 + 3*5.761687*d0*d0 - .010416*18)
    weights = np.array([.209, .253, .311, .132])/.905
    rates = np.array([-1/12.17, -1/59.52, -1/236.5, 0.])
    a = np.diag(rates)
    q, v = [], np.sqrt(weights)
    for i in range(4):
        q.append(v)
        z = a @ v
        for old in q:
            z -= old*(old @ z)
        if i < 3:
            v = z/np.linalg.norm(z)
    q = np.array(q).T
    j = q.T @ a @ q
    _, eigenvectors = eigh(j)
    null = np.abs(eigenvectors[:, -1])
    capacity = 2.12*dp/.905*(null/null[0])**2
    carbon_per_m = 3.62e14*12e-15  # Gt C / (mol m^-3 * m)
    depths = capacity/carbon_per_m
    depths[0] -= 2.12*dp/carbon_per_m
    velocities = np.diag(j, 1)*np.sqrt(capacity[:-1]*capacity[1:])/carbon_per_m
    velocities = np.append(velocities, 0.)
    assert np.all(depths > 0) and np.all(velocities[:-1] > 0)
    residual = max(abs(expm(j*t)[0, 0]-np.sum(weights*np.exp(rates*t)))
                   for t in [0., 1., 10., 100., 1000.])
    assert residual < 1e-12
    with (ROOT/'data/ocean_layers.csv').open('w') as f:
        w = csv.writer(f)
        w.writerow(['layer', 'depth_m', 'exchange_below_m_per_yr'])
        for i in range(4):
            w.writerow([i, '%.15g' % depths[i], '%.15g' % velocities[i]])
    (ROOT/'results/ocean_reconstruction.json').write_text(json.dumps({
        'DIC0_mol_m3': d0, 'dpCO2_dDIC': dp,
        'total_depth_m': float(sum(depths)),
        'maximum_linear_IRF_error': residual,
        'status': 'Independent four-mode reduction, not verified original geometry'
    }, indent=2)+'\n')


if __name__ == '__main__':
    main()
