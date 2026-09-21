#include <stdio.h>
#include <string.h>

#include "calendar_ui.h"
#include "calendar_fonts.h"

/* Set to 0 to drop the line under the week counter. */
#define SHOW_DAILY_MESSAGE 1
#define DAILY_MESSAGE      "A NEW DAY BEGINS."

#define SCREEN_WIDTH  400
#define SCREEN_HEIGHT 300

/* The month grid takes the right ~60% of the panel; the left column is the
   remainder, which is what caps the clock at 48 px (see calendar_fonts.h). */

/* Left column: date, clock, environment, week counter, message. */
#define LEFT_X 14
#define LEFT_Y 14
#define LEFT_W 134
#define LEFT_H 226

/* Right column: month grid. */
#define RIGHT_X 168
#define RIGHT_Y 15
#define RIGHT_W 216
#define RIGHT_H 220

#define SEP_V_X 158
#define SEP_V_Y 15
#define SEP_V_H 220

#define SEP_H_X 16
#define SEP_H_Y 247
#define SEP_H_W 368

#define BAR_X 16
#define BAR_Y 252
#define BAR_W 368
#define BAR_H 32

#define CAL_COLS     7
#define CAL_ROWS     6
#define CAL_CELL_W   30
#define CAL_CELL_H   26
#define CAL_GRID_X   3  /* (RIGHT_W - CAL_COLS * CAL_CELL_W) / 2 */
#define CAL_HEADER_Y 30
#define CAL_GRID_Y   54

static lv_obj_t *root;
static lv_obj_t *date_label;
static lv_obj_t *weekday_label;
static lv_obj_t *time_label;
static lv_obj_t *temperature_label;
static lv_obj_t *humidity_label;
static lv_obj_t *week_info_label;
static lv_obj_t *month_label;
static lv_obj_t *calendar_days[CAL_ROWS][CAL_COLS];
static lv_obj_t *battery_label;
static lv_obj_t *bottom_right_label;

static int grid_year;
static int grid_month;
static int grid_day;

static lv_point_t vertical_separator_points[2];
static lv_point_t horizontal_separator_points[2];
static lv_point_t env_separator_points[2];

static const char *const weekday_initials[CAL_COLS] = { "M", "T", "W", "T", "F", "S", "S" };

/* An lv_obj_create() container arrives with a border, a radius, padding and a
   grey fill; on a 1 bit panel all of that turns into stray black. Strip it. */
static void strip_container(lv_obj_t *obj)
{
    lv_obj_set_scrollbar_mode(obj, LV_SCROLLBAR_MODE_OFF);
    lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_border_width(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_radius(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_all(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(obj, LV_OPA_TRANSP, LV_PART_MAIN | LV_STATE_DEFAULT);
}

static lv_obj_t *make_container(lv_obj_t *parent, lv_coord_t x, lv_coord_t y, lv_coord_t w, lv_coord_t h)
{
    lv_obj_t *obj = lv_obj_create(parent);

    lv_obj_set_pos(obj, x, y);
    lv_obj_set_size(obj, w, h);
    strip_container(obj);
    return obj;
}

static lv_obj_t *make_label(lv_obj_t *parent, lv_coord_t x, lv_coord_t y, lv_coord_t w,
                            const lv_font_t *font, lv_text_align_t align,
                            lv_coord_t letter_space, const char *text)
{
    lv_obj_t *label = lv_label_create(parent);

    lv_label_set_long_mode(label, LV_LABEL_LONG_CLIP);
    lv_label_set_text(label, text);
    lv_obj_set_pos(label, x, y);
    lv_obj_set_width(label, w);
    lv_obj_set_style_text_font(label, font, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(label, lv_color_black(), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(label, LV_OPA_COVER, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(label, align, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(label, letter_space, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(label, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(label, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_radius(label, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_all(label, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(label, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(label, LV_OPA_TRANSP, LV_PART_MAIN | LV_STATE_DEFAULT);
    return label;
}

static lv_obj_t *make_line(lv_obj_t *parent, lv_coord_t x, lv_coord_t y, lv_point_t points[2],
                           lv_coord_t dx, lv_coord_t dy)
{
    lv_obj_t *line = lv_line_create(parent);

    points[0].x = 0;
    points[0].y = 0;
    points[1].x = dx;
    points[1].y = dy;
    lv_line_set_points(line, points, 2);
    lv_obj_set_pos(line, x, y);
    lv_obj_set_style_line_width(line, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_line_color(line, lv_color_black(), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_line_opa(line, LV_OPA_COVER, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_line_rounded(line, false, LV_PART_MAIN | LV_STATE_DEFAULT);
    return line;
}

/* Today is the only cell that inverts. */
static void set_day_highlight(lv_obj_t *cell, bool today)
{
    if (cell == NULL) {
        return;
    }
    if (today) {
        lv_obj_set_style_bg_opa(cell, LV_OPA_COVER, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_bg_color(cell, lv_color_black(), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_radius(cell, 3, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_text_color(cell, lv_color_white(), LV_PART_MAIN | LV_STATE_DEFAULT);
    } else {
        lv_obj_set_style_bg_opa(cell, LV_OPA_TRANSP, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_radius(cell, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_text_color(cell, lv_color_black(), LV_PART_MAIN | LV_STATE_DEFAULT);
    }
}

void calendar_ui_create(lv_obj_t *parent)
{
    root = make_container(parent, 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
    lv_obj_set_style_bg_opa(root, LV_OPA_COVER, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(root, lv_color_white(), LV_PART_MAIN | LV_STATE_DEFAULT);

    lv_obj_t *left_panel = make_container(root, LEFT_X, LEFT_Y, LEFT_W, LEFT_H);

    date_label    = make_label(left_panel, 2, 4, 132, &lv_font_calendar_16, LV_TEXT_ALIGN_LEFT, 0, "");
    weekday_label = make_label(left_panel, 2, 28, 132, &lv_font_calendar_12, LV_TEXT_ALIGN_LEFT, 1, "");

    /* Fixed width and left alignment so the digits never shift sideways as the
       minute changes; the clock font's digits share one advance width. */
    time_label = make_label(left_panel, 1, 54, 133, &lv_font_calendar_clock_48, LV_TEXT_ALIGN_LEFT, -2, "--:--");

    lv_obj_t *environment_panel = make_container(left_panel, 2, 120, 132, 22);

    temperature_label = make_label(environment_panel, 0, 2, 58, &lv_font_calendar_16, LV_TEXT_ALIGN_LEFT, 0, "");
    make_line(environment_panel, 62, 3, env_separator_points, 0, 16);
    /* Right-aligned, so the row stays balanced when the reading loses a digit. */
    humidity_label = make_label(environment_panel, 68, 2, 64, &lv_font_calendar_16, LV_TEXT_ALIGN_RIGHT, 0, "");

    week_info_label = make_label(left_panel, 2, 166, 132, &lv_font_calendar_12, LV_TEXT_ALIGN_LEFT, 0, "");
#if SHOW_DAILY_MESSAGE
    make_label(left_panel, 2, 196, 132, &lv_font_calendar_12, LV_TEXT_ALIGN_LEFT, 0, DAILY_MESSAGE);
#endif

    make_line(root, SEP_V_X, SEP_V_Y, vertical_separator_points, 0, SEP_V_H);

    lv_obj_t *right_panel = make_container(root, RIGHT_X, RIGHT_Y, RIGHT_W, RIGHT_H);

    month_label = make_label(right_panel, 0, 0, RIGHT_W, &lv_font_calendar_18, LV_TEXT_ALIGN_LEFT, 0, "");

    for (int col = 0; col < CAL_COLS; col++) {
        make_label(right_panel, CAL_GRID_X + col * CAL_CELL_W, CAL_HEADER_Y,
                   CAL_CELL_W, &lv_font_calendar_16, LV_TEXT_ALIGN_CENTER, 0,
                   weekday_initials[col]);
    }

    for (int row = 0; row < CAL_ROWS; row++) {
        for (int col = 0; col < CAL_COLS; col++) {
            lv_obj_t *cell = make_label(right_panel, CAL_GRID_X + col * CAL_CELL_W,
                                        CAL_GRID_Y + row * CAL_CELL_H, CAL_CELL_W,
                                        &lv_font_calendar_16, LV_TEXT_ALIGN_CENTER, 0, "");
            lv_obj_set_height(cell, CAL_CELL_H);
            /* Centre the 17 px line box in the 26 px cell. */
            lv_obj_set_style_pad_top(cell, 4, LV_PART_MAIN | LV_STATE_DEFAULT);
            calendar_days[row][col] = cell;
        }
    }

    make_line(root, SEP_H_X, SEP_H_Y, horizontal_separator_points, SEP_H_W, 0);

    lv_obj_t *bottom_bar = make_container(root, BAR_X, BAR_Y, BAR_W, BAR_H);

    battery_label      = make_label(bottom_bar, 0, 9, 200, &lv_font_calendar_12, LV_TEXT_ALIGN_LEFT, 1, "BATTERY --%");
    bottom_right_label = make_label(bottom_bar, BAR_W - 200, 9, 200, &lv_font_calendar_12, LV_TEXT_ALIGN_RIGHT, 1, "");
}

lv_obj_t *calendar_ui_root(void)
{
    return root;
}

void calendar_ui_update_time(int hour, int minute, bool valid)
{
    char buf[8];

    if (time_label == NULL) {
        return;
    }
    if (!valid) {
        lv_label_set_text(time_label, "--:--");
        return;
    }
    snprintf(buf, sizeof(buf), "%02d:%02d", hour, minute);
    lv_label_set_text(time_label, buf);
}

void calendar_ui_update_environment(float temperature_c, float humidity_percent, bool valid)
{
    char temp_buf[16];
    char hum_buf[16];

    if (temperature_label == NULL || humidity_label == NULL) {
        return;
    }
    /* Never show 0°C / 0% for a sensor that has not answered yet — it reads as
       real data. */
    if (!valid) {
        lv_label_set_text(temperature_label, "--.-°C");
        lv_label_set_text(humidity_label, "--% RH");
        return;
    }
    snprintf(temp_buf, sizeof(temp_buf), "%.1f°C", temperature_c);
    snprintf(hum_buf, sizeof(hum_buf), "%.0f%% RH", humidity_percent);
    lv_label_set_text(temperature_label, temp_buf);
    lv_label_set_text(humidity_label, hum_buf);
}

void calendar_ui_update_battery(uint8_t percent)
{
    char buf[20];

    if (battery_label == NULL) {
        return;
    }
    snprintf(buf, sizeof(buf), "BATTERY %u%%", percent);
    lv_label_set_text(battery_label, buf);
}

static void clear_calendar_grid(void)
{
    for (int row = 0; row < CAL_ROWS; row++) {
        for (int col = 0; col < CAL_COLS; col++) {
            lv_label_set_text(calendar_days[row][col], "");
            set_day_highlight(calendar_days[row][col], false);
        }
    }
    grid_year = 0;
    grid_month = 0;
    grid_day = 0;
}

static lv_obj_t *calendar_day_cell(int year, int month, int day)
{
    int index = calendar_calc_weekday(year, month, 1) + day - 1;
    int row = index / CAL_COLS;

    if (row >= CAL_ROWS) {
        return NULL;
    }
    return calendar_days[row][index % CAL_COLS];
}

/* Monday-first grid of the current month only: no leading or trailing days. */
static void draw_calendar_grid(const calendar_ui_data_t *data)
{
    char buf[12];
    int first_weekday = calendar_calc_weekday(data->year, data->month, 1);
    int days          = calendar_calc_days_in_month(data->year, data->month);

    if (data->year == grid_year && data->month == grid_month) {
        if (data->day != grid_day) {
            set_day_highlight(calendar_day_cell(grid_year, grid_month, grid_day), false);
            set_day_highlight(calendar_day_cell(data->year, data->month, data->day), true);
            grid_day = data->day;
        }
        return;
    }

    clear_calendar_grid();
    for (int day = 1; day <= days; day++) {
        int index = first_weekday + day - 1;
        int row   = index / CAL_COLS;
        int col   = index % CAL_COLS;

        if (row >= CAL_ROWS) {
            break;
        }
        snprintf(buf, sizeof(buf), "%d", day);
        lv_label_set_text(calendar_days[row][col], buf);
        set_day_highlight(calendar_days[row][col], day == data->day);
    }
    grid_year = data->year;
    grid_month = data->month;
    grid_day = data->day;
}

void calendar_ui_refresh_all(const calendar_ui_data_t *data)
{
    char buf[48];

    if (root == NULL || data == NULL) {
        return;
    }

    calendar_ui_update_time(data->hour, data->minute, data->time_valid);
    calendar_ui_update_environment(data->temperature_c, data->humidity_percent, data->environment_valid);

    if (!data->time_valid) {
        lv_label_set_text(date_label, "");
        lv_label_set_text(weekday_label, "WAITING FOR TIME");
        lv_label_set_text(week_info_label, "");
        lv_label_set_text(month_label, "");
        lv_label_set_text(bottom_right_label, "");
        clear_calendar_grid();
        return;
    }

    snprintf(buf, sizeof(buf), "%04d · %02d · %02d", data->year, data->month, data->day);
    lv_label_set_text(date_label, buf);
    lv_label_set_text(weekday_label, calendar_calc_weekday_name(data->weekday));

    snprintf(buf, sizeof(buf), "WEEK %d · DAY %d", data->week_number, data->day_of_year);
    lv_label_set_text(week_info_label, buf);

    snprintf(buf, sizeof(buf), "%s %04d", calendar_calc_month_name(data->month), data->year);
    lv_label_set_text(month_label, buf);

    snprintf(buf, sizeof(buf), "%d / %d", data->day_of_year, data->days_of_year);
    lv_label_set_text(bottom_right_label, buf);

    draw_calendar_grid(data);
}
