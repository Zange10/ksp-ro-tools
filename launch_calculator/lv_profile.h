struct Stage {
    double F_vac;       // Thrust produced by the engines in a vacuum [N]
    double F_sl;        // Thrust produced by the engines at sea level [N]
    double m0;          // initial mass (mass at t0) [kg]
    double me;          // vessel mass without fuel [kg]
    double burn_rate;   // burn rate of all running engines combined [kg/s]
};


struct LV {
    char * name;            // name of the launch vehicle
    int stage_n;            // amount of stages of the launch vehicle
    double A;               // biggest cross-section (relevant for drag calculation) [m²]
    double c_d;             // drag coefficient
    struct Stage * stages;  // the stages of the launch vehicle
};

// initialize launch vehicle with its stages (without payload mass)
struct  LV init_LV(char * name, int amt_of_stages, struct Stage *stages);
// write temporary parameters of LV to file which will be manually changed by user
void    write_temp_LV_file();
// read LV parameters from file
void    read_LV_from_file(struct LV * lv);
// read LV parameters from file for Test.lv
void    get_test_LV(struct LV * lv);