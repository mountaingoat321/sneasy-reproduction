# Verification record

The C++ verification executable, `scripts/convergence.py`, and
`scripts/analyze.py` enforce complementary checks. See the numerical CSVs for
measured residuals, tolerances, and conditional Monte Carlo diagnostics.

## Failures identified and addressed

1. **Historical forcing lag.** The first implementation imposed observed CO2
   only at step endpoints. Temporal refinement exposed approximately first-order
   error in future carbon/temperature. Applying the prescribed combined
   atmosphere/surface inventory at the RK midpoint removed this lag. At 80
   ocean cells the corrected half-year versus eighth-year difference in 2100
   temperature is about 0.000010 C, compared with about 0.0023 C before correction.
   The ensemble was regenerated after the fix.
2. **Carbonate domain.** The initial full ensemble failed the requirement that
   every trajectory remain within Yi's published DIC domain. One extreme
   proposal exceeded 2.3 mol/m3. The analysis now explicitly excludes invalid
   proposals and records their identifiers and pre-exclusion normalized weight.
   It does not reinterpret the extrapolated row as valid model output. This
   is a restriction of the ensemble measure and is documented, even though its
   historical-summary weight is negligible. The raw proposal CSV retains the
   rejected row for auditing; the figure uses valid members only.
3. **External access.** Journal supplement retrieval, GTSR download, GitHub API
   access, and normal Git index writes failed. These remain unresolved. They
   are not scientific test passes and prevent an exact reproduction/publication.
4. **TeX cache location.** The first PDF build failed while TeX attempted to
   create a format in the read-only home directory. The Makefile now explicitly
   directs TeX formats, configuration, fonts and caches under `build/`. The
   report subsequently compiled without changing system installations.

The reference ocean's linear small-pulse response is checked against the
independently stated Hooss reduced impulse response. Analytic flood losses
are compared with a 20,000-point independent elevation quadrature, including
overtopped and non-overtopped cases. The Antarctic reference radius, volume,
balance and volume derivative are tested separately. These tests constrain
numerical errors without using Figure 1 values as tuning targets.

No claim of posterior convergence is made: there is no Markov chain. Prefix
quantiles, effective sample sizes, effective 5% tail counts, and multiplier
bootstrap standard errors characterize finite importance-sampling precision.
The approximation to the likelihood and prior dominates inference uncertainty.

Physical experiments ran on one CPU. The initial 4,096-member ensemble,
refinement runs, and forcing sensitivity took about 58 seconds and a peak
32 MB resident memory as reported by `/usr/bin/time`. This is a machine-specific
measurement, not a performance guarantee. The corrected full-build timings
are recorded in local build logs.
