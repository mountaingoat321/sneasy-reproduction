# Figure 1 specification and dependency audit

Source: Wong et al. (2025), DOI [10.1038/s44304-025-00089-0](https://doi.org/10.1038/s44304-025-00089-0), supplied manuscript, read independently.

| Item | Specification in the manuscript |
|---|---|
| Quantity | Combined adaptation expenditures and residual economic losses, globally summed |
| Statistic | Median and 5th–95th percentiles, conditional on warming bin |
| Bins | T <= 1.5; 1.5 < T <= 2.5; 2.5 < T <= 3.5; T > 3.5 degrees C |
| Temperature | Year 2100 relative to 1850–1900 mean |
| Ensemble | 5,000 members subsampled from 100,000; counts 202, 2264, 2145, 389 |
| Currency | Constant 2010 US dollars |
| Discount rate | 4% per year |
| Economic years | 2010–2150, 10-year steps |
| Adaptation periods | 2010–2050, 2060–2100, 2110–2150 |
| Socioeconomics | SSP4 in the main analysis; SSP2 sensitivity |
| Geography | 12,148 DIVA segments |
| Sea-level baseline | 1995–2014 in Figure 2; individual component references differ |
| Localization | Component fingerprints on a 1-degree grid plus nonclimatic local trends |
| Storm tides | GTSR; DINAS-COAST used for a sensitivity experiment |
| Decision rule | Initial optimal adaptation spin-up; least NPV in a roughly 50-year window; strategy/return level retained and height updated |
| Cost components | Wetlands, retreat, inundation, flood property/life losses, protection construction/maintenance |

The wording “discounted to the year 2150” in the Figure 1 caption describes
the aggregation horizon; it does not establish a terminal-value calculation.
The exact base date, within-decade quadrature, terminal weights, and treatment
of spin-up expenditures require additional documentation. A year-2010 NPV
is the explicit convention of the independent benchmark, not a recovered detail.

Dependency chain:

```text
Emissions distribution + historical inputs + other forcing
  -> carbon cycle <-> DOECLIM
  -> temperature and ocean heat -> component sea levels
  -> fingerprints + vertical land motion -> local relative sea level
  -> surge distribution + elevation/exposure + SSP4 + cost parameters
  -> adaptation decisions and annual losses -> discounted global sums
  -> condition on each member's 2100 warming -> weighted quantiles
```

Published numbers below come from prose on pages 2–3, not plot digitization.
The two high-temperature 95th percentiles are not given numerically there.
They must remain missing unless recovered from permitted written sources.

| Bin | 5th | Median | 95th | Units |
|---|---:|---:|---:|---|
| <=1.5 | 1.0 | 1.2 | 1.3 | trillion 2010 USD |
| (1.5,2.5] | 1.2 | 1.5 | 2.3 | trillion 2010 USD |
| (2.5,3.5] | 1.6 | 2.1 | unavailable | trillion 2010 USD |
| >3.5 | 2.2 | 2.6 | unavailable | trillion 2010 USD |

## Source access limits

The journal's supplemental-material link was identified and attempted with
the browser. It returned an internal retrieval error. Direct Springer media
and static-content URLs also failed. Therefore the supplement has **not**
been read, and no claim of complete specification recovery is justified.
The later published version of Darnell et al. (2025), DOI
10.1038/s41558-025-02457-0, postdates Wong and cannot automatically specify
the 2024 preprint configuration Wong used. Its data/code archives were not accessed.

Shell network calls fail DNS resolution. In particular, `gh api user --jq .login`
could not reach api.github.com. Browser-accessible source text can establish
equations and small observation tables, but is not a substitute for verified
retrieval of geographic rasters or full calibration datasets.
