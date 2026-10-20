# Final research summary

**Outcome: Figure 1 was not reproduced; GitHub publication is blocked.**

An independent C++ climate–carbon–sea-level implementation and a synthetic
coastal economic benchmark now run from a single offline build. The physical
system includes DOECLIM diffusion, four land carbon pools, a four-reservoir
ocean derived from published response modes, atmospheric CO2, and five
sea-level contributions. The report distinguishes reconstructed equations
from replacement parameters, assumptions, and missing calibration.

Of 4,096 proposals, 4,095 pass the carbonate domain criterion. The weighted
effective sample size is 1,068.6. Across the four published warming bins,
synthetic median costs are 11.039, 11.005, 10.969, and 11.061 **billion**
2010 USD per hypothetical 100 km. Published global medians are 1.2, 1.5,
2.1, and 2.6 **trillion** USD2010. Global-dollar discrepancies cannot be
computed from these incommensurable quantities. The benchmark's flat response
does not recover the published trend; its warmest-bin effective 5% tail count
is only 1.4.

The reference historical warming, ocean heat gain, and sea-level rise differ
from independent assessments by +16.9%, +37.0%, and −2.54%. Conservation,
equilibrium, analytical-reference, and convergence tests constrain numerical
errors. Midpoint boundary forcing was corrected after a refinement failure;
an out-of-domain proposal is retained for audit but excluded from the figure.

Exact reproduction requires the missing written configuration, joint
calibration, geographic sea-level mapping, DIVA exposure, GTSR hazard, SSP4,
and exact cost accounting. Original model code and prepared inputs were never
used. GitHub API requests fail, remote identity/visibility cannot be verified,
and the CLI reports an authentication failure. Meaningful commits survive in
the writable checkpoint Git database and portable bundle.

Rebuild with `make clean all audit`; read `reports/report.pdf`,
`results/figure1_statistics.csv`, and `reports/figure_status.csv`.
