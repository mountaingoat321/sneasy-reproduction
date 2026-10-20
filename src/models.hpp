#pragma once
#include <algorithm>
#include <array>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <numeric>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace model {
constexpr double seconds_year = 365.25*86400.;
constexpr double earth_area = 5.10072e14;
constexpr double ocean_area = 3.62e14;
constexpr double pi = 3.14159265358979323846;
inline double sq(double x) { return x*x; }
inline void require(bool ok, const std::string& message) {
    if (!ok) throw std::runtime_error(message);
}

struct Parameters {
    double ecs=3., kappa=2.2, beta=.55, q10=2., exchange=1.;
    double peak=2050., growth=.015, decline=.06, other=.56;
    double glacier=.00089, gis_c=-3., gis_b=7.8, gis_alpha=.00073, gis_rate=.00014;
    double ais_alpha=.5, ais_gamma=2.5, expansion=1.7e-4;
};

// Kriegler (2005), equations 2.5--2.6. Implicit midpoint finite volumes.
// Temperature is SST in the ocean cells, air temperature over land.
struct Climate {
    int n;
    double dz, dt, land_feedback, sea_feedback, transport;
    std::vector<double> t, lo, diag, hi, inv_pivot, upper;
    Climate(const Parameters& p, double step=.5, int cells=40):
        n(cells+2), dz(4000./cells), dt(step), t(n,0.), lo(n,0.),
        diag(n,0.), hi(n,0.), inv_pivot(n), upper(n) {
        constexpr double fl=.29, b=1.3, ratio=1.43;
        double equilibrium_sea=p.ecs/(fl*ratio+(1-fl)*b);
        double d=(1-b/ratio)/fl;
        land_feedback=(3.7/(ratio*equilibrium_sea)-1.59*d)/(1-.31*d);
        transport=1.59-.31*land_feedback;
        sea_feedback=3.7/equilibrium_sea+transport/(1-fl)*(ratio-b);
        require(transport>0 && land_feedback>0 && sea_feedback>0, "Unphysical DOECLIM parameters");
        double diffusion=p.kappa*1e-4*seconds_year; // cm2/s -> m2/y
        double g=.95*.13*diffusion/(dz/2.);
        diag[0]=-(land_feedback+transport/fl)/.52;
        hi[0]=transport*b/fl/.52;
        lo[1]=transport/(1-fl)/7.8;
        diag[1]=-(sea_feedback+transport*b/(1-fl)+g)/7.8;
        hi[1]=g/7.8;
        double r=diffusion/(dz*dz);
        lo[2]=2*r; diag[2]=-3*r; hi[2]=r;
        for (int j=3;j<n;++j) {
            lo[j]=r; diag[j]=-2*r; hi[j]=r;
        }
        diag[n-1]=-r; hi[n-1]=0.; // insulated lower boundary
        for (int j=0;j<n;++j) {
            double pivot=1-dt/2*diag[j];
            if (j) pivot-=(-dt/2*lo[j])*upper[j-1];
            inv_pivot[j]=1/pivot;
            upper[j]=(-dt/2*hi[j])*inv_pivot[j];
        }
    }
    void step(double forcing) {
        std::vector<double> rhs(n);
        for (int j=0;j<n;++j) {
            rhs[j]=(1+dt/2*diag[j])*t[j];
            if (j) rhs[j]+=dt/2*lo[j]*t[j-1];
            if (j+1<n) rhs[j]+=dt/2*hi[j]*t[j+1];
        }
        rhs[0]+=dt*forcing/.52;
        rhs[1]+=dt*forcing/7.8;
        for (int j=0;j<n;++j) {
            if (j) rhs[j]-=(-dt/2*lo[j])*rhs[j-1];
            rhs[j]*=inv_pivot[j];
        }
        for (int j=n-2;j>=0;--j) rhs[j]-=upper[j]*rhs[j+1];
        t.swap(rhs);
    }
    double gmst() const { return .29*t[0]+.71*1.3*t[1]; }
    double deep_mean() const {
        return std::accumulate(t.begin()+2,t.end(),0.)/(n-2);
    }
    double ohc() const { // YJ; mixed-layer heat capacity includes atmosphere
        return earth_area*.71*(7.8*t[1]+.95*.13*4000*deep_mean())*seconds_year/1e24;
    }
    double heat() const { return ohc()+earth_area*.29*.52*t[0]*seconds_year/1e24; }
    double radiative_loss() const { return .29*land_feedback*t[0]+.71*sea_feedback*t[1]; }
};

// Yi et al. (2001) carbonate polynomial, mol/m3, degrees C, ppm.
inline double pco2(double d) {
    double t=18.; // Ricciuto et al. (2008) fixed-temperature ocean chemistry
    return std::exp(-40.5797+.074487*t-.000283*t*t+63.69519*d
                    -32.04292*d*d+5.761687*d*d*d-.010416*t*d);
}
inline double dpco2(double d) {
    return pco2(d)*(63.69519-2*32.04292*d+3*5.761687*d*d-.010416*18);
}
struct CarbonGeometry {
    std::array<double,4> depth{}, velocity{};
    double d0=2.;
    explicit CarbonGeometry(const std::string& path="data/ocean_layers.csv") {
        std::ifstream in(path); require(bool(in), "Missing ocean geometry: run scripts/prepare.py");
        std::string line; std::getline(in,line);
        for (int i=0;i<4;++i) {
            require(bool(std::getline(in,line)),"Incomplete ocean geometry");
            std::replace(line.begin(),line.end(),',',' ');
            std::istringstream row(line); int index; row>>index>>depth[i]>>velocity[i];
            require(bool(row) && index==i && depth[i]>0,"Invalid ocean geometry");
        }
        for (int i=0;i<12;++i) d0-=(pco2(d0)-278.)/dpco2(d0);
    }
};

// State: combined atmosphere/surface perturbation, three deep perturbations,
// then absolute leaf, wood, detritus, humus carbon. All inventories in Gt C.
struct Carbon {
    const CarbonGeometry& g;
    Parameters p;
    std::array<double,8> m{{0,0,0,0,100,500,120,1500}};
    double max_budget_error=0., min_dic=3., max_dic=0.;
    Carbon(const CarbonGeometry& geom, const Parameters& params):g(geom),p(params) {}
    double top_mass(double d) const {
        return 2.12*(pco2(d)-278.)+ocean_area*12e-15*g.depth[0]*(d-g.d0);
    }
    double dic(double mass) const {
        double d=g.d0+mass/(2.12*dpco2(g.d0)+ocean_area*12e-15*g.depth[0]);
        d=std::clamp(d,1.6,2.8);
        for (int i=0;i<15;++i) {
            double change=(top_mass(d)-mass)/(2.12*dpco2(d)+ocean_area*12e-15*g.depth[0]);
            d-=std::clamp(change,-.1,.1);
            if (std::abs(change)<1e-13) break;
        }
        require(std::abs(top_mass(d)-mass)<1e-6,"Air-sea equilibration did not converge");
        return d;
    }
    double concentration() const { return pco2(dic(m[0])); }
    static double sum(const std::array<double,8>& y) {
        return std::accumulate(y.begin(),y.end(),0.);
    }
    std::array<double,8> rate(const std::array<double,8>& y,double temperature,double emission) const {
        std::array<double,8> f{};
        double d=dic(y[0]), c=pco2(d), resp=std::pow(p.q10,temperature/10.);
        double npp=60*(1+p.beta*std::log(c/278.));
        double leaf=35*y[4]/100., wood=25*y[5]/500.;
        double det=55*y[6]/120.*resp, soil=15*y[7]/1500.*resp;
        f[4]=npp*35/60.-leaf;
        f[5]=npp*25/60.-wood;
        f[6]=leaf+.8*wood-det;
        f[7]=.2*wood+det*10/55.-soil;
        f[0]=emission-npp+det*45/55.+soil;
        std::array<double,4> water{{d,g.d0+y[1]/(ocean_area*12e-15*g.depth[1]),
            g.d0+y[2]/(ocean_area*12e-15*g.depth[2]),
            g.d0+y[3]/(ocean_area*12e-15*g.depth[3])}};
        for (int i=0;i<3;++i) {
            double flux=ocean_area*12e-15*g.velocity[i]*p.exchange*(water[i]-water[i+1]);
            f[i]-=flux; f[i+1]+=flux;
        }
        return f;
    }
    // RK2; a prescribed concentration replaces the external-emission boundary.
    // Return compatible net external emissions including the boundary correction.
    double step(double dt,double temperature,double emission,double prescribed_ppm=0.) {
        double old=sum(m);
        double prescribed_mass=0.;
        if (prescribed_ppm>0.) {
            double d=dic(m[0]);
            for (int i=0;i<12;++i) d-=(pco2(d)-prescribed_ppm)/dpco2(d);
            prescribed_mass=top_mass(d);
        }
        auto k1=rate(m,temperature,emission), mid=m;
        for (int i=0;i<8;++i) mid[i]+=dt/2*k1[i];
        // Apply the observed boundary at the midpoint as well as the endpoint.
        // Otherwise historical land/ocean fluxes incur a first-order forcing lag.
        if (prescribed_ppm>0.) mid[0]=(m[0]+prescribed_mass)/2;
        auto k2=rate(mid,temperature,emission);
        for (int i=0;i<8;++i) m[i]+=dt*k2[i];
        max_budget_error=std::max(max_budget_error,std::abs(sum(m)-old-dt*emission));
        if (prescribed_ppm>0.) {
            m[0]=prescribed_mass;
        }
        double d=dic(m[0]); min_dic=std::min(min_dic,d); max_dic=std::max(max_dic,d);
        for (int i=4;i<8;++i) require(m[i]>0,"Negative terrestrial carbon inventory");
        return (sum(m)-old)/dt;
    }
};

struct Observations {
    std::vector<double> co2;
    Observations() {
        std::ifstream in("data/co2_annual.csv"); require(bool(in),"Missing CO2 observations");
        std::string line; std::getline(in,line);
        while (std::getline(in,line)) {
            std::replace(line.begin(),line.end(),',',' ');
            std::istringstream row(line); int year; double value; row>>year>>value;
            require(year==1959+int(co2.size()),"Nonconsecutive CO2 observations"); co2.push_back(value);
        }
    }
    double historical(double year) const {
        // No purported observations before 1959: explicit smooth spin-up boundary.
        if (year<1959.) return 278.+(co2[0]-278.)*sq((year-1850.)/109.);
        double x=std::clamp(year-1959.,0.,63.);
        int i=int(x); if (i==63) return co2[i];
        return co2[i]*(1-(x-i))+co2[i+1]*(x-i);
    }
};

struct Annual {
    int year;
    double temperature, sst, subsurface, heat, co2, emissions;
    double glacier=0., greenland=0., antarctic=0., thermal=0., landwater=0., sl=0.;
};
inline double mean(const std::vector<Annual>& a,int first,int last,double Annual::*field) {
    double s=0; for (int y=first;y<=last;++y) s+=a.at(y-1850).*field;
    return s/(last-first+1);
}

// Shaffer (2014) DAIS geometry. Positive water depth = sea level - bed.
// The text extraction of eq. 10 has the opposite bed sign; use physical depth.
struct Antarctic {
    static constexpr double b=775., slope=.0006, mu=8.7, ei=917./3083., ew=1030./3083.;
    static double volume(double r,double sea) {
        double rc=(b-sea)/slope;
        double v=pi*(1+ei)*(8./15*std::sqrt(mu)*std::pow(r,2.5)-slope/3*r*r*r);
        if (r>rc) v-=pi*ew*(2./3*slope*(r*r*r-rc*rc*rc)-b*(r*r-rc*rc));
        return v;
    }
    static double derivative(double r) {
        return pi*(1+ei)*(4./3*std::sqrt(mu)*std::pow(r,1.5)-slope*r*r)
             -2*pi*ew*(slope*r*r-b*r);
    }
    static double balance(double r,double air) {
        double precip=.35*std::exp(.04*air), beta=.012*std::sqrt(precip);
        double h=1471+95*air, result=pi*precip*r*r;
        if (h>0) {
            double delta=h-b+slope*r;
            double rr=std::clamp(r-delta*delta/mu,0.,r), width=r-rr;
            result-=pi*beta*delta*(r*r-rr*rr);
            result-=4*pi*beta*std::sqrt(mu)/5*std::pow(width,2.5);
            result+=4*pi*beta*std::sqrt(mu)/3*r*std::pow(width,1.5);
        }
        return result;
    }
    static double discharge(double r,double sea,double ocean,double alpha,double gamma) {
        double h=std::max(0.,slope*r-b+sea), href=slope*1.864e6-b;
        double speed=1.2*((1-alpha)+alpha*sq((ocean+1.8)/2.52))
                     *std::pow(h,gamma)/std::pow(href,gamma-1);
        return -2*pi*r*(1030./917.)*h*speed;
    }
    static double equilibrium_radius() {
        double lo=1.5e6,hi=2.2e6;
        for (int i=0;i<60;++i) {
            double mid=(lo+hi)/2;
            if (balance(mid,-18)+discharge(mid,0,.72,0,1)>0) lo=mid; else hi=mid;
        }
        return (lo+hi)/2;
    }
};

void sea_level(std::vector<Annual>& a,const Parameters& p,double dt);
std::vector<Annual> simulate(const Parameters& p,const CarbonGeometry& g,const Observations& obs,
                             double dt=.5,int cells=40,double* budget=nullptr,double* max_dic=nullptr);

// Synthetic test coast. This is deliberately NOT a CIAM or global cost model.
struct Coast {
    double length_km, slope, asset_usd_m2, population_m2, surge_scale, wetland_fraction;
};
struct Cost {
    double protection=0., relocation=0., inundation=0., flood=0., wetland=0.;
    int protected_segments=0, retreated_segments=0;
    double total() const { return protection+relocation+inundation+flood+wetland; }
    Cost& operator+=(const Cost& b) {
        protection+=b.protection; relocation+=b.relocation; inundation+=b.inundation;
        flood+=b.flood; wetland+=b.wetland;
        protected_segments+=b.protected_segments; retreated_segments+=b.retreated_segments;
        return *this;
    }
};
// E[min((sea+X-z)+ / 2,1) 1(X>height-sea)], X exponential.
// Damage is zero unless a defense is overtopped. No wall breach probabilities.
inline double expected_damage(double z,double sea,double height,double scale) {
    double a=z-sea,b=a+2,threshold=std::max(0.,height-sea);
    double lo=std::max({0.,a,threshold}),hi=std::max(lo,b);
    double result=0.;
    if (hi>lo) result=((lo+scale-a)*std::exp(-lo/scale)
                     -(hi+scale-a)*std::exp(-hi/scale))/2;
    return result+std::exp(-std::max({0.,b,threshold})/scale);
}
inline double damage_integral(double low,double high,double sea,double height,double scale) {
    if (high<=low) return 0.;
    double threshold=std::max(0.,height-sea), split=sea+threshold;
    double probability=std::exp(-threshold/scale), result=0.;
    double end=std::min(high,split-2.);
    if (end>low) { result+=(end-low)*probability; low=end; }
    end=std::min(high,split);
    if (end>low) {
        auto primitive=[&](double z) {
            return probability*((threshold+scale+sea)*z-z*z/2)/2
                 +scale*scale/2*std::exp(-(z-sea+2)/scale);
        };
        result+=primitive(end)-primitive(low); low=end;
    }
    if (high>low) result+=scale*scale/2*(-std::expm1(-2/scale))
        *(std::exp(-(low-sea)/scale)-std::exp(-(high-sea)/scale));
    return std::max(0.,result);
}
Cost coastal_cost(const std::vector<Annual>& a,double discount=.04,double sl_factor=1.,
                  double growth=.01,bool no_adaptation=false);
} // namespace model
