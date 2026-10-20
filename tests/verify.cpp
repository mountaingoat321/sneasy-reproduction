#include "../src/models.hpp"
#include <iostream>
using namespace model;

int main() {
    try {
        std::ofstream out("results/verification.csv");
        out<<"test,value,tolerance,pass\n"<<std::setprecision(14);
        auto check=[&](const std::string& name,double error,double tolerance) {
            bool pass=std::isfinite(error) && std::abs(error)<=tolerance;
            out<<name<<','<<error<<','<<tolerance<<','<<pass<<'\n'; out.flush();
            require(pass,"FAILED: "+name+" error="+std::to_string(error));
        };
        Parameters p; CarbonGeometry g;
        Climate zero(p,.5,40);
        for (int i=0;i<200;++i) zero.step(0.);
        check("zero_forcing_equilibrium",zero.gmst(),1e-14);
        Climate heat(p,.25,80);
        double energy_error=0.;
        for (int i=0;i<400;++i) {
            double h=heat.heat(), loss=heat.radiative_loss();
            heat.step(3.7);
            double expected=.25*(3.7-(loss+heat.radiative_loss())/2)*earth_area*seconds_year/1e24;
            energy_error=std::max(energy_error,std::abs(heat.heat()-h-expected));
        }
        check("global_heat_budget_YJ_per_step",energy_error,1e-11);
        // Exact analytical doubled-CO2 equilibrium inserted in every ocean cell.
        Climate eq(p,.5,40);
        double ts=p.ecs/(.29*1.43+.71*1.3);
        eq.t.assign(eq.n,ts); eq.t[0]=1.43*ts;
        for (int i=0;i<100;++i) eq.step(3.7);
        check("doubled_CO2_equilibrium_C",eq.gmst()-p.ecs,1e-11);
        Carbon c(g,p);
        auto initial=c.m;
        for (int i=0;i<1000;++i) c.step(.25,0.,0.);
        check("carbon_equilibrium_GtC",Carbon::sum(c.m)-Carbon::sum(initial),1e-9);
        check("carbon_equilibrium_ppm",c.concentration()-278.,1e-9);
        double before=Carbon::sum(c.m);
        c.m[0]+=100.;
        for (int i=0;i<400;++i) c.step(.25,1.,0.);
        check("carbon_pulse_conservation_GtC",Carbon::sum(c.m)-before-100,1e-8);
        check("ocean_uptake_positive",c.m[1]>0?0:1,0.);
        Parameters ocean_only=p; ocean_only.beta=0.; ocean_only.q10=1.;
        Carbon pulse(g,ocean_only); pulse.m[0]=.01;
        for (int i=0;i<4000;++i) pulse.step(.025,0.,0.);
        double fraction=2.12*(pulse.concentration()-278.)/.01;
        double analytical=.209*std::exp(-100/12.17)+.253*std::exp(-100/59.52)
                         +.311*std::exp(-100/236.5)+.132;
        check("ocean_linear_pulse_at_100_years",fraction-analytical,2e-5);
        double max_quadrature_error=0.;
        for (double sea:{-.1,.5,3.}) for (double wall:{0.,2.,5.}) {
            double numerical=0.; int n=20000;
            for (int k=0;k<n;++k) numerical+=10./n*expected_damage((k+.5)*10/n,sea,wall,.55);
            max_quadrature_error=std::max(max_quadrature_error,
                std::abs(numerical-damage_integral(0,10,sea,wall,.55)));
        }
        check("flood_integral_against_quadrature_m",max_quadrature_error,1e-7);
        check("no_exposure_no_loss",damage_integral(10,10,2,0,.5),0.);
        check("wall_reduces_flood_risk",expected_damage(1,.5,4,.55)<=expected_damage(1,.5,0,.55)?0:1,0.);
        double r=Antarctic::equilibrium_radius();
        check("DAIS_reference_radius_relative",r/1.864e6-1,.001);
        check("DAIS_reference_volume_relative",Antarctic::volume(r,0)/24.78e15-1,.01);
        double derivative=(Antarctic::volume(r+1,0)-Antarctic::volume(r-1,0))/2;
        check("DAIS_volume_derivative_relative",derivative/Antarctic::derivative(r)-1,1e-7);
        double flow=Antarctic::balance(r,-18)+Antarctic::discharge(r,0,.72,0,1);
        check("DAIS_reference_balance_relative",flow/Antarctic::balance(r,-18),1e-12);
        Observations obs;
        double budget,dic;
        auto a=simulate(p,g,obs,.5,40,&budget,&dic);
        check("coupled_carbon_budget_GtC",budget,1e-8);
        check("sea_level_component_sum_m",a[250].sl-a[250].glacier-a[250].greenland-
              a[250].antarctic-a[250].thermal-a[250].landwater,1e-12);
        check("SL_reference_baseline_m",mean(a,1995,2014,&Annual::sl),1e-12);
        check("temperature_reference_baseline_C",mean(a,1850,1900,&Annual::temperature),1e-12);
        check("historical_CO2_boundary_ppm",a[172].co2-418.53,1e-8);
        auto cost=coastal_cost(a);
        check("cost_categories_nonnegative",std::min({cost.protection,cost.relocation,cost.inundation,cost.flood,cost.wetland})>=0?0:1,0.);
        check("discount_sensitivity",coastal_cost(a,.02).total()>cost.total()
            &&cost.total()>coastal_cost(a,.06).total()?0:1,0.);
        // For the test reference, adaptation also reduces full-horizon losses.
        check("reference_adaptation_benefit",cost.total()<=coastal_cost(a,.04,1,.01,100,true).total()?0:1,0.);
        std::cout<<"All verification checks passed. DAIS radius="<<r<<" m\n";
    } catch (const std::exception& e) { std::cerr<<e.what()<<'\n'; return 1; }
}
