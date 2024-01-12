#ifndef KSP_EPHEM_H
#define KSP_EPHEM_H

/**
 * @brief Represents the ephemeral data consisting of date, position, and velocity components
 */
struct Ephem {
	double date; /**< Date associated with the ephemeral data (Julian Date) */
	double x;    /**< X-coordinate position */
	double y;    /**< Y-coordinate position */
	double z;    /**< Z-coordinate position */
	double vx;   /**< Velocity along the X-axis */
	double vy;   /**< Velocity along the Y-axis */
	double vz;   /**< Velocity along the Z-axis */
};

/**
 * @brief Represents a date and time with year, month, day, hour, minute, and second components
 */
struct Date {
	int y;       /**< Year */
	int m;       /**< Month */
	int d;       /**< Day */
	int h;       /**< Hour */
	int min;     /**< Minute */
	double s;    /**< Seconds */
};


/**
 * @brief Prints date in the format YYYY-MM-DD hh:mm:ss.f
 *
 * @param date The date to be printed
 * @param line_break Is 0 if no line break should follow and 1 if otherwise
 */
void print_date(struct Date date, int line_break);


/**
 * @brief Converts date format to Julian Date
 *
 * @param date The date to be converted
 *
 * @return The Julian Date
 */
double convert_date_JD(struct Date date);


/**
 * @brief Converts Julian Date to date format
 *
 * @param JD The Julian Date to be converted
 *
 * @return The resulting Date struct
 */
struct Date convert_JD_date(double JD);


/**
 * @brief Prints the date, position and velocity vector of the given ephemeris
 *
 * @param ephem The given ephemeris
 */
void print_ephem(struct Ephem ephem);


/**
 * @brief Retrieves the ephemeral data of requested body for requested time (from JPL's Horizon API or from file)
 *
 * @param ephem The array where all the ephemeral data is to be stored
 * @param size_ephem The biggest size of the ephemeris array
 * @param body_code The body code of the requested celestial body (from JPL's Horizon Application)
 * @param time_steps The size of the time steps between each ephemeris
 * @param jd0 The earliest requested date (Julian Date)
 * @param jd1 The latest requested date (Julian Date)
 * @param download Is 1 if the data is to be retrieved from JPL's Horizon API and is 0 if the data is to be retrieved from file
 */
void get_ephem(struct Ephem *ephem, int size_ephem, int body_code, int time_steps, double jd0, double jd1, int download);



void print_ephem_to_file(char *file_path, int body_code, double jd0, double jd1);

/**
 * @brief Find the last ephemeris before the given date
 *
 * @param ephem List of all ephemeral data
 * @param date The given date (Julian Date)
 *
 * @return The last ephemeris before given date
 */
struct Ephem get_closest_ephem(struct Ephem *ephem, double date);

#endif //KSP_EPHEM_H
