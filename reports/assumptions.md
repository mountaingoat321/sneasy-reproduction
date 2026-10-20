# Configuration, assumptions, and limitations

## Physical system

Time is in years of 365.25 days. Heat fluxes are W/m2, temperatures degrees C,
carbon inventories Gt C, emissions Gt CO2/year (converted by 12/44), atmospheric
CO2 ppm, and sea level metres. 1 YJ = 10^24 J. Atmospheric capacity is 2.12 Gt
C/ppm, Earth area 5.10072e14 m2, ocean area 3.62e14 m2. The small difference
between ocean area and 0.71 Earth area is retained from rounded source constants.

| Component | Fixed values | Source status |
|---|---|---|
| DOECLIM | land fraction 0.29; active ocean 0.95; depth 4000 m; land/sea heat capacities 0.52/7.8 W yr m-2 K-1; volumetric heat capacity 0.13 W yr m-3 K-1; marine-air/SST ratio 1.3; land/SST equilibrium ratio 1.43; exchange 1.59−0.31 lambda_land; doubled CO2 forcing 3.7 W/m2 | Kriegler's written equations; finite-volume solver implemented here |
| Carbonate chemistry | Yi polynomial; fixed 18 C, salinity 35, alkalinity 2.35 meq/kg; preindustrial 278 ppm | Polynomial from Yi; fixed temperature variant follows Ricciuto; 278 rather than its 280 ppm is an explicit baseline choice |
| Land carbon | initial pools [100,500,120,1500] Gt C; NPP 60; leaf/wood allocation 35/25; mortality 35/25; wood transfers 20/5 to litter/soil; litter transfers 45/10 to air/soil; soil respiration 15 Gt C/yr | DCESS II totals replace unrecovered SNEASY stock constants; no land-use geography or nutrient limitation |
| Ocean carbon | atmosphere equilibrates instantly with surface; three deep reservoirs; generated depths [64.09,420.46,1140.88,2389.51] m and velocities [16.95,9.08,6.34] m/yr | Independent positive-reservoir reduction of published Hooss response; no claim of identical original SNEASY geometry |
| Historical boundary | 1850 initial equilibrium; quadratic CO2 increase 278 ppm to 315.98 ppm in 1959, then NOAA interpolation through 2022 | Pre-1959 curve is assumed, not observations; concentration prescribed thereafter, so agreement is not validation |
| Other forcing | linear 0 in 1850 to 0.56 W/m2 in 2019, constant later | Illustrative net non-CO2 forcing motivated by AR6's 2019 total anthropogenic minus CO2 ERF; not the paper's SSP matching or a historical forcing reconstruction |
| Future emissions | 40.7 Gt CO2/yr in 2022; published integer-year growth/decline recurrence | Starting value is an illustrative total, not a retrieved calibrated draw; recurrence retained exactly after its start and not clamped. Discrete decline leaves a small positive asymptote, unlike its continuous integral |
| Glacier | beta 0.00089 m/(yr C), available volume 0.4 m, exponent 0.78, equilibrium temperature −0.15 C | BRICK written-model values; initialize loss to zero in **1850**, an explicit departure from calibrated 1990 initial condition |
| Greenland | equilibrium volume clip(7.8−3T,0,8.832) m; rate max(0,0.00073T+0.00014)/yr; initial 7.4 m in **1850** | SIMPLE form and published parameter summaries; rate/volume bounds and earlier initialization explicit model-domain assumptions, not recovered Wong setup; T relative to 1961–1990 |
| Antarctica | Shaffer geometry/constants, reference radius solved at −18 C and subsurface 0.72 C; polar temperature = −18+1.8 deltaT; ocean temperature = 0.72+250-m anomaly | Polar amplification and global-250-m-to-local mapping **assumed**; no rapid-disintegration threshold, paleoclimate calibration, ice-shelf geography, or cavity-volume correction |
| Thermal expansion | constant effective expansion coefficient 1.7e-4 K-1; volumetric water heat capacity 4.1e6 J m-3 K-1 | Physical bulk approximation, not recovered BRICK calibrated thermal-expansion configuration; mixed-layer OHC includes a small atmospheric contribution |
| Land water | +0.3 mm/yr after 1961 | Explicit illustrative constant, not a reconstructed reservoir/groundwater time series |

Ocean carbon and ocean heat mixing are independent, as in the cited SNEASY
architecture. Ocean carbon temperature feedback is disabled in this experiment.
The Antarctic sign interpretation and the absolute regional-temperature mapping
are material uncertainties. Its water-equivalent mass change is converted to
sea level using density ratio 917/1030 and ocean area, without correcting for
changing marine ice cavities. Thus the system is **partially reconstructed**,
not a verified reimplementation of calibrated SNEASY–BRICK.

## Ensemble measure

4,096 independent proposals use MT19937 seed 20251089. The first 256 also form
the pilot. Parameters below are independent before weighting; these are declared
sensitivity distributions, not a reconstructed joint posterior.

| Parameter | Distribution | Interpretation |
|---|---|---|
| ECS | Uniform 1.5–6 C | broad sensitivity range, not a cited posterior |
| heat diffusivity | Uniform 0.5–4 cm2/s | restricted plausible range within BRICK prior |
| fertilization beta | Uniform 0.287–0.8 | spans published reference and larger carbon response |
| Q10 | Uniform 1.2–2.5 | illustrative respiration uncertainty |
| carbon exchange multiplier | Log-uniform 0.5–2, applied to all three exchanges | original sensitivity assumption; not only thermocline mixing |
| emissions peak | Discrete uniform 2023–2100 | policy scenarios, not calibrated probabilities |
| emissions growth coefficient | Uniform 0–0.03 Gt CO2/yr3 | annual recurrence assumption |
| decline coefficient | Log-uniform 0.02–0.15 /yr | annual recurrence assumption |
| glacier beta | Uniform 0.00059–0.0013 m/(yr C) | uses published marginal interval as bounds, **not as a posterior distribution** |
| Antarctic alpha/gamma | Uniform 0–1 / 0.5–4.25 | Shaffer configuration range, no paleoclimate likelihood |
| thermal expansion | Uniform 1.2e-4–2.2e-4 /K | original bulk-parameter sensitivity |

Unnormalized weight is exp{−0.5[(T2010–19−1.06)/0.15]^2
−0.5[(deltaOHC1971–2018−0.396)/0.10]^2}. The denominators are **assumed total
discrepancy scales**, deliberately wider than pure observational errors. The
summaries are treated as independent. No internal variability model, correlated
time-series likelihood, MCMC, carbon calibration, sea-level calibration, or
independent socioeconomic draw is used. Effective sample size is (sum w)^2/sum w2.
Reported probabilities and intervals apply only to this artificial measure.
Intervals use the inverse weighted empirical CDF without smoothing. A 400-draw
exponential multiplier bootstrap estimates Monte Carlo standard errors.

## Synthetic coastal accounting

Three non-geographic segments total 100 km. Elevation is a plane from 0 to
10 m above the 2010 water level; area per vertical metre equals length/slope.
There are no global fingerprints or vertical land motion. Factors 0.8 and 1.2
are perturbations of the same sea-level curve, not geographic fingerprint data.

| Length km | Slope | Assets USD2010/m2 | Persons/m2 | Exponential surge scale m | Wetland fraction |
|---:|---:|---:|---:|---:|---:|
| 40 | .002 | 150 | .0002 | .35 | .30 |
| 35 | .005 | 600 | .002 | .55 | .10 |
| 25 | .010 | 1800 | .008 | .75 | .02 |

All these values are hypothetical. Annual maximum surge is exponential; its
R-year return level is scale*log(R). Damage fraction grows linearly from zero
to one over 2 m of flooding. A wall suppresses all losses until overtopping;
breach, wave action, and compound events are omitted. Expected losses are
integrated analytically over both surge and elevation.

Property exposure, moving expenses, and life valuation grow at 1%/yr in real
terms, with fixed population. Mobile capital is 25%; proactive movement costs
that capital plus USD30,000/person. The remaining 75% of capital is lost at
retreat/inundation. Reactive moving costs multiply proactive costs by five.
Expected mortality is 0.001 times flood damage fraction, valued at USD3 million
per person. Dike capital costs are USD5 million/km times height squared, with
1% annual maintenance; upgrades pay the difference in squared heights. All
monetary constants are hypothetical 2010 dollars, not inferred CIAM coefficients.
Wetland services are USD2/m2/yr over a 2-m elevation band. Loss fraction is
min[(positive sea-level rise/2)^2,1], or one behind protection (coastal squeeze).
Retreated capital never returns; new assets are not explicitly constructed.

At 2010 choose inaction, five retreat return levels (1,10,100,1000,10000), or
four protection return levels (10,100,1000,10000), by minimizing 2010–2059 NPV.
Hold the strategy and return level, updating height in 2060 and 2110 to the
maximum sea level over the next 50 years (last window ends 2150). Costs are
annual, inclusive 2010–2150, paid at each year end, and discounted to start-2010
at 4%. Initial adaptation is charged; there is no excluded spin-up. This differs
from Wong's ten-year accounting and incompletely recovered spin-up convention.
The no-rise counterfactual retains all present-day surge risk and economic growth;
the main figure shows **total**, not counterfactual-subtracted, costs.

The fixed initial strategy is optimal only over its first window. Full-horizon
costs need not be globally optimal. Constant geometry, full protection until
overtopping, immediate adaptation, and perfect sea-level foresight are strong
limitations. This benchmark tests the coupling and conditional-statistics
machinery; it cannot supply absolute or relative errors in global dollars.
