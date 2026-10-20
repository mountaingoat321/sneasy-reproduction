#include "models.hpp"

namespace model {
std::vector<Annual> simulate(const Parameters& p,const CarbonGeometry& g,const Observations& obs,
                             double dt,int cells,double* budget,double* max_dic) {
    int steps=int(std::round(1/dt));
    require(std::abs(steps*dt-1)<1e-12,"Timestep must divide one year");
    Climate climate(p,dt,cells);
    Carbon carbon(g,p);
    std::vector<Annual> a;
    double q=40.7, q_peak=q, previous_emission=0.; // Gt CO2 / y, scenario assumption
    for (int year=1850;year<=2150;++year) {
        double x=250./climate.dz-.5;
        int cell=std::max(0,int(std::floor(x)));
        double fraction=std::max(0.,x-cell);
        double subsurface=climate.t[cell+2]*(1-fraction)+climate.t[cell+3]*fraction;
        a.push_back({year,climate.gmst(),climate.t[1],subsurface,climate.ohc(),
                     carbon.concentration(),previous_emission});
        if (year==2150) break;
        // Wong's annual recurrence, applied at interval start. Negative emissions,
        // if produced by discretization, are retained rather than silently clipped.
        if (year>=2023) {
            if (year<=p.peak) { q+=p.growth*(p.peak-year); q_peak=q; }
            else {
                double e=std::exp(-p.decline*(year-p.peak));
                q-=2*q_peak*p.decline*e/sq(1+e);
            }
        }
        double annual_emissions=0.;
        for (int j=0;j<steps;++j) {
            double time=year+(j+.5)*dt;
            double prescribed=(time<2022)?obs.historical(year+(j+1)*dt):0.;
            double e=(time<2022)?0.:q*12/44.;
            double c0=carbon.concentration(), t0=climate.gmst();
            auto initial=carbon.m;
            carbon.step(dt,t0,e,prescribed); // predictor for midpoint forcing
            double c1=carbon.concentration();
            double forcing=3.7/std::log(2.)*std::log(std::sqrt(c0*c1)/278.);
            forcing+=p.other*std::clamp((time-1850.)/169.,0.,1.);
            climate.step(forcing);
            carbon.m=initial;
            annual_emissions+=carbon.step(dt,(t0+climate.gmst())/2,e,prescribed)*dt;
        }
        previous_emission=annual_emissions*44/12.;
    }
    double baseline=mean(a,1850,1900,&Annual::temperature);
    for (auto& y:a) y.temperature-=baseline;
    if (budget) *budget=carbon.max_budget_error;
    if (max_dic) *max_dic=carbon.max_dic;
    sea_level(a,p,dt);
    return a;
}

void sea_level(std::vector<Annual>& a,const Parameters& p,double dt) {
    // Initial ice inventories are benchmark assumptions at 1850, not calibrated.
    double gic=0., gis=7.4, r=Antarctic::equilibrium_radius();
    double ice_change=0., other_previous=0.;
    double air_base=mean(a,1961,1990,&Annual::temperature);
    double ocean_base=mean(a,1961,1990,&Annual::subsurface);
    double gic_base=mean(a,1850,1870,&Annual::temperature);
    double convert=917./1030./ocean_area;
    auto other=[&](double glacier,double greenland,double heat,double year) {
        double thermal=p.expansion*heat*1e24/(4.1e6*ocean_area);
        return glacier+7.4-greenland+thermal+.0003*std::max(0.,year-1961.);
    };
    for (size_t i=0;i<a.size();++i) {
        auto& y=a[i];
        y.glacier=gic; y.greenland=7.4-gis; y.antarctic=-convert*ice_change;
        y.thermal=p.expansion*y.heat*1e24/(4.1e6*ocean_area);
        y.landwater=.0003*std::max(0.,y.year-1961.);
        y.sl=y.glacier+y.greenland+y.antarctic+y.thermal+y.landwater;
        if (i+1==a.size()) break;
        for (int j=0;j<int(std::round(1/dt));++j) {
            double f=(j+.5)*dt, time=y.year+f;
            double t=y.temperature*(1-f)+a[i+1].temperature*f;
            double to=y.subsurface*(1-f)+a[i+1].subsurface*f;
            double heat=y.heat*(1-f)+a[i+1].heat*f;
            auto gic_rate=[&](double s) {
                return p.glacier*(t-gic_base+.15)*std::pow(std::max(0.,1-s/.4),.78);
            };
            double dg=gic_rate(gic+dt/2*gic_rate(gic));
            // Nonnegative relaxation rate; no extrapolation to negative timescales.
            double rate=std::max(0.,p.gis_alpha*(t-air_base)+p.gis_rate);
            double veq=std::clamp(p.gis_b+p.gis_c*(t-air_base),0.,8.832);
            double dv=(veq-gis)*(-std::expm1(-rate*dt));
            double o=other(gic+dt/2*dg,gis+dv/2,heat,time);
            double odot=(o-other_previous)/dt;
            if (i==0 && j==0) odot=0.;
            other_previous=o;
            auto radius_rate=[&](double radius,double volume_change) {
                double sea=o-convert*volume_change;
                double mass=Antarctic::balance(radius,-18+1.8*(t-air_base))
                    +Antarctic::discharge(radius,sea,.72+to-ocean_base,p.ais_alpha,p.ais_gamma);
                double rc=(Antarctic::b-sea)/Antarctic::slope;
                double vsea=-2*pi*Antarctic::ew*(rc*rc-Antarctic::b/Antarctic::slope*rc);
                double sea_rate=odot-convert*mass;
                return std::array<double,2>{{(mass-vsea*sea_rate)/Antarctic::derivative(radius),mass}};
            };
            auto k1=radius_rate(r,ice_change);
            auto k2=radius_rate(r+dt/2*k1[0],ice_change+dt/2*k1[1]);
            r+=dt*k2[0]; ice_change+=dt*k2[1]; gic+=dt*dg; gis+=dv;
            require(r>1.4e6 && r<2.2e6 && std::isfinite(ice_change),"DAIS left verified marine domain");
        }
    }
    // Sea-level outputs use the same declared 1995--2014 reference for all runs.
    for (double Annual::*field : {&Annual::glacier,&Annual::greenland,&Annual::antarctic,
                                  &Annual::thermal,&Annual::landwater,&Annual::sl}) {
        double baseline=mean(a,1995,2014,field);
        for (auto& y:a) y.*field-=baseline;
    }
}

static Cost strategy(const std::vector<Annual>& a,const Coast& c,int mode,int return_years,
                     double discount,double sl_factor,double growth,int last_year=2150) {
    // mode 0: inaction with reactive relocation; 1: retreat; 2: protection.
    // Annual costs 2010--2150 inclusive, paid at year end; decisions 2010/2060/2110.
    Cost cost;
    double crest=0., withdrawn=0., previous_inundated=0.;
    double area_per_m=c.length_km*1000/c.slope;
    double surge=c.surge_scale*std::log(double(return_years));
    double initial_sea=a[2010-1850].sl;
    for (int year=2010;year<=last_year;++year) {
        double sea=sl_factor*(a[year-1850].sl-initial_sea);
        double factor=std::pow(1+growth,year-2010);
        double df=std::pow(1+discount,-(year-2010+1));
        if (year==2010 || year==2060 || year==2110) {
            int end=std::min(2150,year+49);
            double maximum=sea;
            for (int k=year;k<=end;++k) maximum=std::max(maximum,sl_factor*(a[k-1850].sl-initial_sea));
            double design=std::max(0.,maximum+surge);
            if (mode==2) {
                // Incremental quadratic construction cost, USD 5 million / km / m^2.
                double next=std::max(crest,design);
                cost.protection+=df*5e6*c.length_km*(next*next-crest*crest);
                crest=next;
            }
            if (mode==1) {
                double next=std::clamp(std::max(withdrawn,design),0.,10.);
                double area=(next-withdrawn)*area_per_m;
                cost.relocation+=df*factor*area*(.25*c.asset_usd_m2+30000*c.population_m2);
                cost.inundation+=df*factor*area*.75*c.asset_usd_m2;
                withdrawn=next;
            }
        }
        if (mode==2) cost.protection+=df*.01*5e6*c.length_km*crest*crest;
        double exposed_low=std::max(withdrawn,mode==2?0.:std::max(0.,sea));
        double newly_flooded=std::max(0.,std::min(10.,exposed_low)-std::max(withdrawn,previous_inundated));
        if (mode!=2 && newly_flooded>0.) {
            double area=newly_flooded*area_per_m;
            cost.inundation+=df*factor*area*.75*c.asset_usd_m2;
            cost.relocation+=df*factor*area*5*(.25*c.asset_usd_m2+30000*c.population_m2);
        }
        previous_inundated=std::max(previous_inundated,exposed_low);
        // Integrate exposure over elevations 0--10 m. Losses are recurring repair costs.
        double damage=damage_integral(exposed_low,10.,sea,mode==2?crest:0.,c.surge_scale);
        cost.flood+=df*factor*area_per_m*damage*(.75*c.asset_usd_m2+.001*3e6*c.population_m2);
        double wetland_loss=mode==2?1.:std::clamp(sq(std::max(0.,sea)/2),0.,1.);
        cost.wetland+=df*wetland_loss*area_per_m*2*c.wetland_fraction*2.; // USD 2/m2/y service
    }
    cost.protected_segments=(mode==2); cost.retreated_segments=(mode==1);
    return cost;
}

Cost coastal_cost(const std::vector<Annual>& a,double discount,double sl_factor,double growth,
                  bool no_adaptation) {
    const std::array<Coast,3> coast{{
        {40,.002,150,.0002,.35,.3},
        {35,.005,600,.002,.55,.1},
        {25,.010,1800,.008,.75,.02}
    }};
    Cost sum;
    for (const auto& c:coast) {
        Cost best=strategy(a,c,0,1,discount,sl_factor,growth);
        double best_initial=strategy(a,c,0,1,discount,sl_factor,growth,2059).total();
        if (!no_adaptation) {
            for (int r:{1,10,100,1000,10000}) {
                for (int mode:{1,2}) {
                    if (mode==2 && r==1) continue;
                    double initial=strategy(a,c,mode,r,discount,sl_factor,growth,2059).total();
                    if (initial<best_initial) {
                        best_initial=initial;
                        best=strategy(a,c,mode,r,discount,sl_factor,growth);
                    }
                }
            }
        }
        sum+=best;
    }
    return sum;
}
} // namespace model
