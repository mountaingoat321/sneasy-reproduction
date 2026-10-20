#include "models.hpp"
#include <iostream>
#include <random>

using namespace model;

void save_trajectory(const std::string& path,const std::vector<Annual>& a) {
    std::ofstream f(path); require(bool(f),"Cannot write trajectory"); f<<std::setprecision(12);
    f<<"year,temperature_C,sst_C,subsurface_C,ohc_YJ,co2_ppm,emissions_GtCO2_yr,glacier_m,greenland_m,antarctic_m,thermal_m,landwater_m,sea_level_m\n";
    for (const auto& y:a) f<<y.year<<','<<y.temperature<<','<<y.sst<<','<<y.subsurface<<','<<y.heat
        <<','<<y.co2<<','<<y.emissions<<','<<y.glacier<<','<<y.greenland<<','<<y.antarctic
        <<','<<y.thermal<<','<<y.landwater<<','<<y.sl<<'\n';
}

int main(int argc,char** argv) {
    try {
        std::string mode=argc>1?argv[1]:"reference";
        CarbonGeometry geometry; Observations observations; Parameters p;
        if (mode=="reference") {
            double dt=argc>2?std::stod(argv[2]):.5;
            int cells=argc>3?std::stoi(argv[3]):40;
            std::string name=argc>4?argv[4]:"reference";
            double budget, dic;
            auto a=simulate(p,geometry,observations,dt,cells,&budget,&dic);
            save_trajectory("results/"+name+".csv",a);
            auto c=coastal_cost(a);
            std::cout<<std::setprecision(12)<<"T2100="<<a[250].temperature<<" S2100="<<a[250].sl
                     <<" cost_USD="<<c.total()<<" carbon_error_GtC="<<budget<<" max_DIC="<<dic<<'\n';
        } else if (mode=="ensemble") {
            int count=argc>2?std::stoi(argv[2]):256;
            std::string path=argc>3?argv[3]:"results/ensemble.csv";
            std::mt19937 rng(20251089);
            // Explicitly convert raw MT19937 draws for reproducibility across C++ libraries.
            auto unit=[&](){ return (double(rng())+.5)/4294967296.; };
            auto uniform=[&](double lo,double hi){ return lo+(hi-lo)*unit(); };
            auto loguniform=[&](double lo,double hi){ return std::exp(uniform(std::log(lo),std::log(hi))); };
            std::ofstream f(path); require(bool(f),"Cannot write ensemble"); f<<std::setprecision(12);
            f<<"id,ecs_C,kappa_cm2_s,beta,q10,exchange_factor,peak_year,growth,decline_per_yr,glacier_coeff,ais_alpha,ais_gamma,expansion,temperature_2010_2019_C,ohc_1971_2018_YJ,log_weight,temperature_2100_C,sl_2100_m,sl_2150_m,sl_1901_2018_m,co2_2025_ppm,max_DIC,budget_error_GtC,bin,cost_usd2010,protection_usd2010,relocation_usd2010,inundation_usd2010,flood_usd2010,wetland_usd2010,no_adaptation_usd2010,discount2pct_usd2010,discount6pct_usd2010,slfactor08_usd2010,slfactor12_usd2010,baseline_usd2010\n";
            for (int i=0;i<count;++i) {
                p.ecs=uniform(1.5,6.); p.kappa=uniform(.5,4.);
                p.beta=uniform(.287,.8); p.q10=uniform(1.2,2.5); p.exchange=loguniform(.5,2.);
                p.peak=2023+std::floor(78*unit()); p.growth=uniform(0,.03); p.decline=loguniform(.02,.15);
                p.glacier=uniform(.00059,.0013); p.ais_alpha=uniform(0,1); p.ais_gamma=uniform(.5,4.25);
                p.expansion=uniform(1.2e-4,2.2e-4);
                double budget,dic;
                auto a=simulate(p,geometry,observations,.5,40,&budget,&dic);
                double th=mean(a,2010,2019,&Annual::temperature), oh=a[168].heat-a[121].heat;
                // Deliberately loose independent Gaussian summary constraints.
                // These are not Wong's observational likelihood or joint posterior.
                double logw=-.5*sq((th-1.06)/.15)-.5*sq((oh-.396)/.10);
                double t=a[250].temperature;
                int bin=t<=1.5?0:(t<=2.5?1:(t<=3.5?2:3));
                auto c=coastal_cost(a);
                auto baseline=a;
                for (auto& y:baseline) y.sl=a[160].sl;
                f<<i<<','<<p.ecs<<','<<p.kappa<<','<<p.beta<<','<<p.q10<<','<<p.exchange<<','<<p.peak
                 <<','<<p.growth<<','<<p.decline<<','<<p.glacier<<','<<p.ais_alpha<<','<<p.ais_gamma<<','<<p.expansion
                 <<','<<th<<','<<oh<<','<<logw<<','<<t<<','<<a[250].sl<<','<<a[300].sl
                 <<','<<a[168].sl-a[51].sl<<','<<a[175].co2<<','<<dic<<','<<budget<<','<<bin
                 <<','<<c.total()<<','<<c.protection<<','<<c.relocation<<','<<c.inundation<<','<<c.flood<<','<<c.wetland
                 <<','<<coastal_cost(a,.04,1.,.01,100,true).total()
                 <<','<<coastal_cost(a,.02).total()<<','<<coastal_cost(a,.06).total()
                 <<','<<coastal_cost(a,.04,.8).total()<<','<<coastal_cost(a,.04,1.2).total()
                 <<','<<coastal_cost(baseline).total()<<'\n';
                if (i%256==0) std::cerr<<"Completed "<<i+1<<" / "<<count<<"\n";
            }
        } else if (mode=="sensitivity") {
            for (double forcing:{0.,.56,1.12}) {
                p.other=forcing;
                auto a=simulate(p,geometry,observations);
                save_trajectory("results/forcing_"+std::to_string(int(std::round(forcing*100)))+".csv",a);
            }
        } else throw std::runtime_error("Unknown run mode");
    } catch (const std::exception& e) { std::cerr<<e.what()<<'\n'; return 1; }
}
