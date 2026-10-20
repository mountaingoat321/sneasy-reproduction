"""Analyze independently computed ensembles and draw original figures.

Weighted intervals describe this project's sensitivity distribution. They are
not intervals from the published calibration and are not global coastal costs.
"""
import csv
import json
from pathlib import Path
import numpy as np
import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as plt

ROOT = Path(__file__).resolve().parents[1]
LABELS = [r'$T\leq1.5$', r'$1.5<T\leq2.5$', r'$2.5<T\leq3.5$', r'$T>3.5$']


def quantile(x, w, probabilities=(.05, .5, .95)):
    order = np.argsort(x)
    x, w = x[order], w[order]
    cdf = np.cumsum(w)/w.sum()
    return x[np.minimum(np.searchsorted(cdf, probabilities), len(x)-1)]


def ess(w):
    return w.sum()**2 / np.dot(w, w)


def write_csv(path, rows):
    with path.open('w') as f:
        writer = csv.DictWriter(f, fieldnames=list(rows[0]))
        writer.writeheader()
        writer.writerows(rows)


def main():
    x = np.genfromtxt(ROOT/'results/ensemble.csv', delimiter=',', names=True)
    ref = np.genfromtxt(ROOT/'results/reference.csv', delimiter=',', names=True)
    assert len(x) == 4096 and np.all(np.isfinite(x.view(float)))
    assert np.all(x['max_DIC'] <= 2.3), 'Carbonate polynomial outside published domain'
    assert np.max(x['budget_error_GtC']) < 1e-8
    assert np.all(x['cost_usd2010'] >= 0)
    categories = sum(x[k+'_usd2010'] for k in ['protection', 'relocation', 'inundation', 'flood', 'wetland'])
    assert np.max(np.abs(categories-x['cost_usd2010'])/x['cost_usd2010']) < 1e-10
    w = np.exp(x['log_weight']-x['log_weight'].max())
    rng = np.random.RandomState(20251020)
    rows, comparison, convergence, sensitivity = [], [], [], []
    published = np.genfromtxt(ROOT/'data/published_figure1.csv', delimiter=',', names=True, dtype=None, encoding='utf-8')
    low_median = None
    for b in range(4):
        select = x['bin'] == b
        xb, wb = x[select], w[select]
        q = quantile(xb['cost_usd2010'], wb)/1e9
        # Multiplier bootstrap resamples the importance-sampling empirical measure.
        # It measures finite-draw uncertainty, not omitted structural uncertainty.
        boot = np.array([quantile(xb['cost_usd2010'], wb*rng.exponential(size=len(wb)))/1e9
                         for _ in range(400)])
        se = boot.std(axis=0, ddof=1)
        row = dict(bin=b, n=len(xb), effective_n=ess(wb), probability=wb.sum()/w.sum(),
                   q05_billion_usd2010=q[0], median_billion_usd2010=q[1], q95_billion_usd2010=q[2],
                   q05_mc_se_billion=se[0], median_mc_se_billion=se[1], q95_mc_se_billion=se[2],
                   effective_tail_n=.05*ess(wb))
        rows.append(row)
        if low_median is None:
            low_median = q[1]
        pubratio=published['median_trillion_usd2010'][b]/published['median_trillion_usd2010'][0]
        comparison.append(dict(bin=b, published_global_median_trillion=published['median_trillion_usd2010'][b],
                               independent_global_median_trillion='NA', absolute_global_error_trillion='NA',
                               relative_global_error_percent='NA', benchmark_median_billion=q[1],
                               published_median_ratio_to_bin0=pubratio, benchmark_median_ratio_to_bin0=q[1]/low_median,
                               ratio_difference=q[1]/low_median-pubratio,
                               probability_difference_percentage_points=100*(wb.sum()/w.sum()-published['n'][b]/5000),
                               status='NOT_REPRODUCED_INCOMMENSURABLE_GEOGRAPHY'))
        for key in ['no_adaptation','discount2pct','discount6pct','slfactor08','slfactor12','baseline']:
            qs=quantile(xb[key+'_usd2010'],wb)/1e9
            sensitivity.append(dict(bin=b, scenario=key,q05_billion=qs[0],median_billion=qs[1],q95_billion=qs[2],
                                    median_change_percent=100*(qs[1]/q[1]-1)))
        for n in [256,512,1024,2048,4096]:
            subset = (x['id']<n) & select
            qs=quantile(x['cost_usd2010'][subset],w[subset])/1e9
            convergence.append(dict(ensemble_n=n,bin=b,effective_n=ess(w[subset]),
                                    q05_billion=qs[0],median_billion=qs[1],q95_billion=qs[2]))
    write_csv(ROOT/'results/figure1_statistics.csv',rows)
    write_csv(ROOT/'results/published_comparison.csv',comparison)
    write_csv(ROOT/'results/ensemble_convergence.csv',convergence)
    write_csv(ROOT/'results/sensitivities.csv',sensitivity)

    validation=[]
    for name,key,target,index_range,offset in [
        ('2010-2019 temperature vs 1850-1900','temperature_C',1.06,(160,170),None),
        ('1971-2018 ocean heat change YJ','ohc_YJ',.396,(168,169),121),
        ('1901-2018 sea-level change m','sea_level_m',.20,(168,169),51),
        ('2025 CO2 ppm forward holdout','co2_ppm',427.35,(175,176),None)]:
        value=ref[key][slice(*index_range)].mean()
        if offset is not None: value-=ref[key][offset]
        validation.append(dict(quantity=name,reference_model=value,independent_assessment=target,
                               absolute_error=value-target,relative_error_percent=100*(value/target-1)))
    write_csv(ROOT/'results/historical_validation.csv',validation)
    metrics=dict(n=len(x),effective_n=ess(w),maximum_dic=float(x['max_DIC'].max()),
                 maximum_carbon_budget_error_GtC=float(x['budget_error_GtC'].max()),
                 baseline_billion=float(x['baseline_usd2010'][0]/1e9),
                 weighted_temperature_2010_2019=quantile(x['temperature_2010_2019_C'],w).tolist(),
                 weighted_ohc_1971_2018=quantile(x['ohc_1971_2018_YJ'],w).tolist(),
                 weighted_sl_1901_2018=quantile(x['sl_1901_2018_m'],w).tolist(),
                 weighted_co2_2025=quantile(x['co2_2025_ppm'],w).tolist())
    (ROOT/'results/summary.json').write_text(json.dumps(metrics,indent=2)+'\n')

    plt.rcParams.update({'font.size':10,'axes.spines.top':False,'axes.spines.right':False,
                         'savefig.dpi':180,'pdf.fonttype':42})
    fig, ax=plt.subplots(figsize=(7.5,5.1))
    q=np.array([[r['q05_billion_usd2010'],r['median_billion_usd2010'],r['q95_billion_usd2010']] for r in rows])
    for i,color in enumerate(['#246b8e','#2f8b83','#d09927','#ae5345']):
        ax.errorbar(i,q[i,1],yerr=[[q[i,1]-q[i,0]],[q[i,2]-q[i,1]]],fmt='o',color=color,
                    markersize=8,capsize=6,linewidth=2)
        ax.annotate('ESS %.0f'%rows[i]['effective_n'],(i,q[i,2]),xytext=(0,10),textcoords='offset points',ha='center',fontsize=9)
    ax.axhline(metrics['baseline_billion'],color='.55',linestyle=':',label='No sea-level change benchmark')
    ax.set_xticks(range(4));ax.set_xticklabels(LABELS)
    ax.set_xlabel('2100 warming relative to 1850–1900 (°C)')
    ax.set_ylabel('2010–2150 costs (billion 2010 USD per synthetic 100 km)')
    ax.set_ylim(0,max(q[:,2])*1.23);ax.set_xlim(-.5,3.5)
    ax.legend(frameon=False,loc='lower right',fontsize=9)
    ax.set_title('NON-REPRODUCTION: synthetic coastline',color='#9f3028',weight='bold',pad=16)
    fig.text(.5,.025,'Medians and 5th–95th weighted ensemble intervals • 4% discount rate\n'
             'No global exposure, SSP4, calibrated joint posterior, or geographic fingerprints',ha='center',fontsize=9)
    fig.tight_layout(rect=(0,.09,1,1))
    fig.savefig(ROOT/'figures/figure1_nonreproduction.pdf')
    fig.savefig(ROOT/'figures/figure1_nonreproduction.png')
    plt.close(fig)

    fig,axes=plt.subplots(2,2,figsize=(8.2,6.2))
    for ax,key,label in zip(axes.flat,['temperature_C','co2_ppm','ohc_YJ','sea_level_m'],
                           ['Global warming (°C)','Atmospheric CO₂ (ppm)','Ocean heat anomaly (YJ)','Sea level vs 1995–2014 (m)']):
        ax.plot(ref['year'],ref[key],color='#246b8e',label='Reference assumptions')
        for value,style in [(0,'--'),(112,':')]:
            alt=np.genfromtxt(ROOT/('results/forcing_%d.csv'%value),delimiter=',',names=True)
            ax.plot(alt['year'],alt[key],style,color='.55',label='Other forcing %.2f W/m²'%(value/100))
        ax.axvline(2022,color='.7',lw=.8);ax.set_ylabel(label);ax.set_xlabel('Year')
    axes[0,0].legend(fontsize=7,frameon=False)
    fig.suptitle('Independent physical benchmark: assumed forcing sensitivities',fontsize=11)
    fig.tight_layout(rect=(0,0,1,.95));fig.savefig(ROOT/'figures/physical_diagnostics.pdf');plt.close(fig)

    # Small generated LaTeX fragment keeps report numbers tied to saved results.
    tex=['% Generated by scripts/analyze.py; do not edit.',
         r'\newcommand{\TotalESS}{%.1f}'%metrics['effective_n'],
         r'\newcommand{\MaxDIC}{%.4f}'%metrics['maximum_dic'],
         r'\newcommand{\BaselineCost}{%.3f}'%metrics['baseline_billion'],
         r'\begin{tabular}{rrrrrr}',r'\toprule',
         r'Bin & Draws & ESS & Probability & Median & 5th--95th \\',r'\midrule']
    for r in rows:
        tex.append('%d & %d & %.1f & %.3f & %.3f & %.3f--%.3f \\\\' %
                   (r['bin']+1,r['n'],r['effective_n'],r['probability'],r['median_billion_usd2010'],
                    r['q05_billion_usd2010'],r['q95_billion_usd2010']))
    tex += [r'\bottomrule',r'\end{tabular}']
    (ROOT/'reports/generated.tex').write_text('\n'.join(tex)+'\n')
    print(json.dumps(metrics,indent=2))
    print('All ensemble integrity checks passed; four conditional distributions saved.')


if __name__=='__main__':
    main()
