// Mars lander simulator
// Version 1.11
// Header file
// Gabor Csanyi and Andrew Gee, August 2019

// Permission is hereby granted, free of charge, to any person obtaining
// a copy of this software and associated documentation, to make use of it
// for non-commercial purposes, provided that (a) its original authorship
// is acknowledged and (b) no modified versions of the source code are
// published. Restriction (b) is designed to protect the integrity of the
// exercise for future generations of students. The authors would be happy
// to receive any suggested modifications by private correspondence to
// ahg@eng.cam.ac.uk and gc121@eng.cam.ac.uk.

#ifdef _WIN32
#define _USE_MATH_DEFINES
#include <windows.h>
#else
#include <sys/time.h>
#include <unistd.h>
#endif
#ifdef __APPLE__
#include <GLUT/glut.h>
#else
#include <GL/glut.h>
#endif
#include <iostream>
#include <string>
#include <sstream>
#include <fstream>
#include <cmath>
#include <math.h>
#include <cstdlib>
#include <vector>

#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <GL/glui.h>

// GLUT mouse wheel operations work under Linux only
#if !defined (GLUT_WHEEL_UP)
#define GLUT_WHEEL_UP 3
#define GLUT_WHEEL_DOWN 4
#endif

// Graphics constants
#define GAP 5
#define SMALL_NUM 0.0000001
#define N_RAND 20000
#define PREFERRED_WIDTH 1024
#define PREFERRED_HEIGHT 768
#define MIN_INSTRUMENT_WIDTH 1024
#define INSTRUMENT_HEIGHT 300
#define GROUND_LINE_SPACING 20.0
#define CLOSEUP_VIEW_ANGLE 30.0
#define TRANSITION_ALTITUDE 10000.0
#define TRANSITION_ALTITUDE_NO_TEXTURE 4000.0
#define TERRAIN_TEXTURE_SIZE 1024
#define INNER_DIAL_RADIUS 65.0
#define OUTER_DIAL_RADIUS 75.0
#define MAX_DELAY 160000
#define N_TRACK 1000
#define TRACK_DISTANCE_DELTA 100000.0
#define TRACK_ANGLE_DELTA 0.999
#define HEAT_FLUX_GLOW_THRESHOLD 1000000.0

// Mars constants
#define MARS_RADIUS 3386000.0 // (m)
#define MARS_MASS 6.42E23 // (kg)
#define GRAVITY 6.673E-11 // (m^3/kg/s^2)
#define MARS_DAY 88642.65 // (s)
#define EXOSPHERE 200000.0 // (m)

// Lander constants
#define LANDER_SIZE 1.0 // (m)
#define UNLOADED_LANDER_MASS 100.0 // (kg)
#define FUEL_CAPACITY 100.0 // (l)
//#define FUEL_RATE_AT_MAX_THRUST 0.5 // (l/s) originally 0.5
#define FUEL_DENSITY 1.0 // (kg/l)
// MAX_THRUST, as defined below, is 1.5 * weight of fully loaded lander at surface
#define MAX_THRUST (1.5 * (FUEL_DENSITY*FUEL_CAPACITY+UNLOADED_LANDER_MASS) * (GRAVITY*MARS_MASS/(MARS_RADIUS*MARS_RADIUS))) // (N)
#define ENGINE_LAG 0.0 // (s)
#define ENGINE_DELAY 0.0 // (s)
#define DRAG_COEF_CHUTE 2.0
#define DRAG_COEF_LANDER 1.0
#define MAX_PARACHUTE_DRAG 20000.0 // (N)
#define MAX_PARACHUTE_SPEED 500.0 // (m/s)
#define THROTTLE_GRANULARITY 20 // for manual control
#define MAX_IMPACT_GROUND_SPEED 1.0 // (m/s)
#define MAX_IMPACT_DESCENT_RATE 1.0 // (m/s)

using namespace std;

void glut_print (float x, float y, string s);

class indicator_lamp {
public:
  double tcx, tcy, width, height;
  string on_text, off_text, extra_text = "";
  bool on;
    indicator_lamp() {tcx = 0; tcy= 0; width = 0; height = 0; on_text = ""; off_text = ""; extra_text = ""; on = false;}
  indicator_lamp(double cx, double cy, string off, string on, bool boolean,  double w, double h, string extra="") {
    tcx = cx; tcy= cy; width = w; height = h; on_text = on; off_text = off; extra_text = extra; on = boolean;
  }
  void draw() {
    if (on) glColor3f(0.5, 0.0, 0.0);
    else glColor3f(0.0, 0.5, 0.0);
    glBegin(GL_QUADS);
    glVertex2d(tcx-(width/2-0.5), tcy-height+0.5);
    glVertex2d(tcx+(width/2-0.5), tcy-height+0.5);
    glVertex2d(tcx+(width/2-0.5), tcy-0.5);
    glVertex2d(tcx-(width/2-0.5), tcy-0.5);
    glEnd();
    glColor3f(1.0, 1.0, 1.0);
    glBegin(GL_LINE_LOOP);
    glVertex2d(tcx-(width/2), tcy-height);
    glVertex2d(tcx+(width/2), tcy-height);
    glVertex2d(tcx+(width/2), tcy);
    glVertex2d(tcx-(width/2), tcy);
    glEnd();
    if (on) glut_print(tcx-65.0, tcy-14.0, on_text + extra_text);
    else glut_print(tcx-65.0, tcy-14.0, off_text);
  }
  bool is_clicked(int x, int y) {
    bool clicked;
    if (x>=(tcx-(width/2)) && x<=(tcx+(width/2)) && y>=(INSTRUMENT_HEIGHT-tcy) && y<=(INSTRUMENT_HEIGHT-(tcy-height))){
      clicked = true;
    } else {
      clicked = false;
    }
    return clicked;
  }
private:
};

class vector3d {
  // Utility class for three-dimensional vector operations
public:
  vector3d() {x=0.0; y=0.0; z=0.0;}
  vector3d (double a, double b, double c=0.0) {x=a; y=b; z=c;}
  bool operator== (const vector3d &v) const { if ((x==v.x)&&(y==v.y)&&(z==v.z)) return true; else return false; }
  bool operator!= (const vector3d &v) const { if ((x!=v.x)||(y!=v.y)||(z!=v.z)) return true; else return false; }
  vector3d operator+ (const vector3d &v) const { return vector3d(x+v.x, y+v.y, z+v.z); }
  vector3d operator- (const vector3d &v) const { return vector3d(x-v.x, y-v.y, z-v.z); }
  friend vector3d operator- (const vector3d &v) { return vector3d(-v.x, -v.y, -v.z); }
  vector3d& operator+= (const vector3d &v) { x+=v.x; y+=v.y; z+=v.z; return *this; }
  vector3d& operator-= (const vector3d &v) { x-=v.x; y-=v.y; z-=v.z; return *this; }
  vector3d operator^ (const vector3d &v) const { return vector3d(y*v.z-z*v.y, z*v.x-x*v.z, x*v.y-y*v.x); }
  double operator* (const vector3d &v) const { return (x*v.x + y*v.y +z*v.z); }
  friend vector3d operator* (const vector3d &v, const double &a) { return vector3d(v.x*a, v.y*a, v.z*a); }
  friend vector3d operator* (const double &a, const vector3d &v) { return vector3d(v.x*a, v.y*a, v.z*a); }
  vector3d& operator*= (const double &a) { x*=a; y*=a; z*=a; return *this; }
  vector3d operator/ (const double &a) const { return vector3d(x/a, y/a, z/a); }
  vector3d& operator/= (const double &a) { x/=a; y/=a; z/=a; return *this; }
  double abs2() const { return (x*x + y*y + z*z); }
  double abs() const { return sqrt(this->abs2()); }
  vector3d norm() const { double s(this->abs()); if (s==0) return *this; else return vector3d(x/s, y/s, z/s); }
  friend ostream& operator << (ostream &out, const vector3d &v) { out << v.x << ' ' << v.y << ' ' << v.z; return out; }
  double x, y, z;
private:
};

// Data type for recording lander's previous positions
struct track_t {
  unsigned short n;
  unsigned short p;
  vector3d pos[N_TRACK];
};

// Quaternions for orbital view transformation
struct quat_t {
  vector3d v;
  double s;
};

// Data structure for the state of the close-up view's coordinate system
struct closeup_coords_t {
  bool initialized;
  bool backwards;
  vector3d right;
};

// Enumerated data type for parachute status
enum parachute_status_t { NOT_DEPLOYED = 0, DEPLOYED = 1, LOST = 2 };

// Enumerated data type for autopilot status
enum autopilot_t {LAND_MODE = 0, INJECT_MODE = 1};

#ifdef DECLARE_GLOBAL_VARIABLES // actual declarations of all global variables for lander_graphics.cpp

// GL windows and objects
int main_window, closeup_window, orbital_window, instrument_window, view_width, view_height, win_width, win_height;
GLUquadricObj *quadObj;
GLuint terrain_texture;
short throttle_control;
track_t track;
bool texture_available;

// Simulation parameters
bool help = false;
bool paused = false;
bool landed = false;
bool crashed = false;
int last_click_x = -1;
int last_click_y = -1;
short simulation_speed = 5;
double delta_t, simulation_time, error;
unsigned short scenario = 0;
string scenario_description[10];
bool static_lighting = false;
closeup_coords_t closeup_coords;
float randtab[N_RAND];
bool do_texture = true;
unsigned long throttle_buffer_length, throttle_buffer_pointer;
double *throttle_buffer = NULL;
unsigned long long time_program_started;

// Lander state - the visualization routines use velocity_from_positions, so not sensitive to
// any errors in the velocity update in numerical_dynamics
vector3d position, orientation, velocity, velocity_from_positions, last_position, previous_position, drag_force_lander, drag_force_chute, grav_force, acceleration, major_unit, minor_unit, ang_momentum, velocity_wrt_atm;
double climb_speed, ground_speed_wrt_mars, ground_speed_abs, altitude, throttle, fuel, eccentricity, semi_major, semi_minor, orbit_energy, stabilized_attitude_angle, tot_mass, r_p, r_a, theta, cos_theta;
int periapsis = MARS_RADIUS+100000,apoapsis = MARS_RADIUS+100000, circular_orbit;
int autopilot_mode = 0;
bool stabilized_attitude, autopilot_enabled, parachute_lost, show_pred_traj, reached_circular_orbit, reached_final_orbit;
parachute_status_t parachute_status;

// Define fuel rate at max thrust that is not a const so that it can be changed for infinite fuel mode
double fuel_rate_at_max_thrust = 0.5;

// Set values of K_h, K_p and delta
float K_h = 0.019;
float K_p = 2.0;
float delta = 0.5;

GLUI            *glui;
GLUI_Checkbox   *circular_checkbox;
GLUI_EditText   *peri_spinner, *apo_spinner;
GLUI_Spinner    *kh_spinner, *kp_spinner, *delta_spinner;
GLUI_RadioGroup *radio;
GLUI_RadioButton*orbit_radio;
GLUI_Button     *start_button;
GLUI_StaticText *error_text, *scenario_text;

//Optimal values for 10km descent K_h = 0.025, K_p = 2.0, delta = 1.0

// Orbital and closeup view parameters
double orbital_zoom, save_orbital_zoom, closeup_offset, closeup_xr, closeup_yr, terrain_angle;
quat_t orbital_quat;

// For GL lights
GLfloat plus_y[] = { 0.0, 1.0, 0.0, 0.0 };
GLfloat minus_y[] = { 0.0, -1.0, 0.0, 0.0 };
GLfloat plus_z[] = { 0.0, 0.0, 1.0, 0.0 };
GLfloat top_right[] = { 1.0, 1.0, 1.0, 0.0 };
GLfloat straight_on[] = { 0.0, 0.0, 1.0, 0.0 };

#else // extern declarations of those global variables used in lander.cpp

extern bool stabilized_attitude, autopilot_enabled, reached_circular_orbit, reached_final_orbit;
extern double delta_t, simulation_time, throttle, fuel, altitude, ground_speed_wrt_mars, ground_speed_absw, climb_speed, stabilized_attitude_angle, tot_mass, eccentricity, semi_major, semi_minor, r_p, r_a, orbit_energy, theta, cos_theta;
extern short simulation_speed;
extern float K_h, K_p, delta;
extern int autopilot_mode, periapsis, apoapsis;
extern unsigned short scenario;
extern string scenario_description[];
extern vector3d position, orientation, velocity, previous_position, drag_force_lander, drag_force_chute, grav_force, acceleration, major_unit, minor_unit, ang_momentum, velocity_wrt_atm;
extern parachute_status_t parachute_status;
//extern autopilot_t autopilot_mode;
extern double fuel_rate_at_max_thrust;
//extern indicator_lamp autopilot_lamp, att_stabilizer_lamp, parachute_lamp;
#endif

// Function prototypes
void invert (double m[], double mout[]);
void xyz_euler_to_matrix (vector3d ang, double m[]);
vector3d matrix_to_xyz_euler (double m[]);
void normalize_quat (quat_t &q);
quat_t axis_to_quat (vector3d a, const double phi);
double project_to_sphere (const double r, const double x, const double y);
quat_t add_quats (quat_t q1, quat_t q2);
void quat_to_matrix (double m[], const quat_t q);
quat_t track_quats (const double p1x, const double p1y, const double p2x, const double p2y);
void microsecond_time (unsigned long long &t);
void fghCircleTable (double **sint, double **cost, const int n);
void glutOpenHemisphere (GLdouble radius, GLint slices, GLint stacks);
void glutMottledSphere (GLdouble radius, GLint slices, GLint stacks);
void glutCone (GLdouble base, GLdouble height, GLint slices, GLint stacks, bool closed);
void enable_lights (void);
void setup_lights (void);
double atmospheric_density (vector3d pos);
void draw_dial (double cx, double cy, double val, string title, string units);
void draw_control_bar (double tlx, double tly, double val, double red, double green, double blue, string title);
void draw_indicator_lamp (double tcx, double tcy, string off_text, string on_text, bool on, string extra_text);
void draw_instrument_window (void);
void display_help_arrows (void);
void display_help_prompt (void);
void display_help_text (void);
void draw_orbital_window (void);
void draw_parachute_quad (double d);
void draw_parachute (double d);
bool generate_terrain_texture (void);
void update_closeup_coords (void);
void draw_closeup_window (void);
void draw_main_window (void);
void refresh_all_subwindows (void);
bool safe_to_deploy_parachute (void);
void update_visualization (void);
void attitude_stabilization (void);
vector3d thrust_wrt_world (void);
void autopilot_land (void);
void autopilot_inject (void);
void numerical_dynamics (void);
void initialize_simulation (void);
void update_lander_state (void);
void reset_simulation (void);
void set_orbital_projection_matrix (void);
void reshape_main_window (int width, int height);
void orbital_mouse_button (int button, int state, int x, int y);
void orbital_mouse_motion (int x, int y);
void closeup_mouse_button (int button, int state, int x, int y);
void closeup_mouse_motion (int x, int y);
void glut_special (int key, int x, int y);
void glut_key (unsigned char k, int x, int y);
void myGlutIdle(void);
void control_bd(int control);
void show_autopilot_controls (bool on);

double control_function(double x, double x1, double y1, double x2, double y2);
