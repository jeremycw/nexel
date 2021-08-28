#include <unistd.h>
#include "munit.h"
#include "../src/editor.h"
#include "../src/data.h"
#include "../src/sdl_renderer.h"
#include "../src/debugbreak.h"

static MunitResult test_editor_toggle_grid(const MunitParameter params[], void* data) {
  (void)params;
  (void)data;

  struct din_editor_toggle_grid din = { .grid_enabled = 0 };
  struct dout_editor_toggle_grid dout =  editor_toggle_grid(din);

  munit_assert(dout.grid_enabled == 1);

  din = (struct din_editor_toggle_grid) { .grid_enabled = 1 };
  dout = editor_toggle_grid(din);

  munit_assert(dout.grid_enabled == 0);

  return MUNIT_OK;
}

static MunitResult test_editor_copy_selection(const MunitParameter params[], void* data) {
  (void)params;
  (void)data;

  struct indexed_bitmap indexed_bitmap[2];

  struct din_editor_copy_selection din = {
    .dout = {
      .copies = indexed_bitmap,
      .copies_meta = { .size = 0, .capacity = 2, .type_size = sizeof(struct indexed_bitmap) }
    },
    .sdl_renderer = NULL,
    .palette_colours = (uint32_t[]) { [0] = 0xFF0000FF, [1] = 0x00FF00FF, [2] = 0x0000FFFF, [3] = 0x000000FF },
    .image_scale = 4,
    .image_translation = (struct point) { .x = 3, .y = 3 },
    .selections_n = 2,
    .selections = (struct selection[]) {
      [0] = (struct selection) {
        .start = { .x = 3, .y = 3 },
        .end = { .x = 8, .y = 12 }
      },
      [1] = (struct selection) {
        .start = { .x = 7, .y = 3 },
        .end = { .x = 11, .y = 11 }
      }
    },
    .image_pixels_n = 4,
    .image_pixels = (indexed_pixel_t[]){ 0, 1, 2, 3 },
    .image_width = 2
  };

  struct dout_editor_copy_selection dout = editor_copy_selection(din);

  // there should be two copies made from selections
  munit_assert(array_len(dout.copies) == 2);

  // each should be 1x2
  munit_assert(dout.copies[0].width == 1);
  munit_assert(dout.copies[1].width == 1);
  munit_assert(dout.copies[0].height == 2);
  munit_assert(dout.copies[1].height == 2);

  // check that their indexed pixels are correct
  munit_assert(dout.copies[0].pixels[0] == 0);
  munit_assert(dout.copies[0].pixels[1] == 2);
  munit_assert(dout.copies[1].pixels[0] == 1);
  munit_assert(dout.copies[1].pixels[1] == 3);

  // check the the correct rgb values are getting to the sdl surface
  munit_assert(((uint32_t*)dout.copies[0].sdl_bitmap.surface->pixels)[0] == 0xFF0000FF);
  munit_assert(((uint32_t*)dout.copies[0].sdl_bitmap.surface->pixels)[1] == 0x0000FFFF);
  munit_assert(((uint32_t*)dout.copies[1].sdl_bitmap.surface->pixels)[0] == 0x00FF00FF);
  munit_assert(((uint32_t*)dout.copies[1].sdl_bitmap.surface->pixels)[1] == 0x000000FF);

  return MUNIT_OK;
}

static MunitResult test_editor_apply_paste(const MunitParameter params[], void* data) {
  (void)params;
  (void)data;

  struct din_editor_apply_paste din = {
    .image_scale = 4,
    .image_translation = (struct point) { .x = 3, .y = 3 },
    .selections_n = 2,
    .selections = (struct selection[]) {
      [0] = (struct selection) {
        .start = { .x = 3, .y = 3 },
        .end = { .x = 8, .y = 12 }
      },
      [1] = (struct selection) {
        .start = { .x = 7, .y = 3 },
        .end = { .x = 11, .y = 11 }
      }
    },
    .copies = (struct indexed_bitmap[]) {
      [0] = {
        .width = 1,
        .height = 2,
        .pixels = (indexed_pixel_t[]){ 0, 2 }
      },
      [1] = {
        .width = 1,
        .height = 2,
        .pixels = (indexed_pixel_t[]){ 1, 3 }
      }
    },
    .mouse = { .x = 7, .y = 3 },
    .image_pixels = (indexed_pixel_t[]){ 0, 1, 2, 3, 4, 5 },
    .image_width = 3,
    .editor_block_size = 1
  };

  struct dout_editor_apply_paste dout = editor_apply_paste(din);

  // there should be two copies made from selections
  munit_assert(array_len(dout.pixel_changes) == 4);

  munit_assert(dout.pixel_changes[0].buffer_index == 1);
  munit_assert(dout.pixel_changes[0].from == 1);
  munit_assert(dout.pixel_changes[0].to == 0);

  munit_assert(dout.pixel_changes[1].buffer_index == 4);
  munit_assert(dout.pixel_changes[1].from == 4);
  munit_assert(dout.pixel_changes[1].to == 2);

  munit_assert(dout.pixel_changes[2].buffer_index == 2);
  munit_assert(dout.pixel_changes[2].from == 2);
  munit_assert(dout.pixel_changes[2].to == 1);

  munit_assert(dout.pixel_changes[3].buffer_index == 5);
  munit_assert(dout.pixel_changes[3].from == 5);
  munit_assert(dout.pixel_changes[3].to == 3);

  return MUNIT_OK;
}

static MunitResult test_editor_transform_selection(const MunitParameter params[], void* data) {
  (void)params;
  (void)data;

  struct din_editor_transform_selection din = {
    .sdl_renderer = NULL,
    .copies_n = 1,
    .palette_colours = (uint32_t[]) { [0] = 0xFF0000FF, [1] = 0x00FF00FF, [2] = 0x0000FFFF, [3] = 0x000000FF },
    .copies = (struct indexed_bitmap[]) {
      [0] = {
        .width = 1,
        .height = 2,
        .pixels = (indexed_pixel_t[]){ 0, 1 }
      }
    }
  };

  struct dout_editor_transform_selection dout = editor_transform_selection(rotate_clockwise_transform, din);

  munit_assert(dout.transformed_copies[0].width == 2);
  munit_assert(dout.transformed_copies[0].height == 1);

  munit_assert(dout.transformed_copies[0].pixels[0] == 1);
  munit_assert(dout.transformed_copies[0].pixels[1] == 0);

  return MUNIT_OK;
}

static MunitResult test_editor_paint(const MunitParameter params[], void* data) {
  (void)params;
  (void)data;

  struct din_editor_paint din = {
    .image_transform = { .translation = { .x = 2, .y = 1 }, .scale = 2 },
    .mouse = { .x = 5, .y = 1 },
    .image_width = 2,
    .image_pixels = (indexed_pixel_t[]) { 0, 1, 2, 3 },
    .image_pixels_n = 4,
    .paint_colour = 5
  };

  struct dout_editor_paint dout = editor_paint(EDITOR_BEGIN_UNDO_GROUP, din);

  munit_assert(dout.begin_undoable_group == EDITOR_BEGIN_UNDO_GROUP);

  munit_assert(dout.pixel_change.buffer_index == 1);
  munit_assert(dout.pixel_change.from == 1);
  munit_assert(dout.pixel_change.to == 5);

  return MUNIT_OK;
}

static MunitResult test_editor_pick_image_colour(const MunitParameter params[], void* data) {
  (void)params;
  (void)data;

  struct din_editor_pick_image_colour din = {
    .image_transform = { .translation = { .x = 2, .y = 1 }, .scale = 2 },
    .mouse = { .x = 5, .y = 1 },
    .image_width = 2,
    .image_pixels = (indexed_pixel_t[]) { 0, 1, 2, 3 },
    .image_pixels_n = 4,
    .paint_colour = 5
  };

  struct dout_editor_pick_image_colour dout = editor_pick_image_colour(din);

  munit_assert(dout.paint_colour == 1);

  return MUNIT_OK;
}

static MunitTest tests[] = {
  // definition order: test-name, test-func, setup-func, teardown-func, options, params
  { (char*) "/editor/toggle_grid", test_editor_toggle_grid, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL },
  { (char*) "/editor/copy_selection", test_editor_copy_selection, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL },
  { (char*) "/editor/apply_paste", test_editor_apply_paste, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL },
  { (char*) "/editor/transform_selection", test_editor_transform_selection, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL },
  { (char*) "/editor/paint", test_editor_paint, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL },
  { (char*) "/editor/pick_image_colour", test_editor_pick_image_colour, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL },
  // end
  { NULL, NULL, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL }
};

static const MunitSuite test_suite = {
  (char*) "",             // Test suite name prefix
  tests,                  // Tests
  NULL,                   // Sub test suites
  1,                      // Execution mode (normal)
  MUNIT_SUITE_OPTION_NONE // Options
};

int main(int argc, char** argv) {
  return munit_suite_main(&test_suite, (void*) "nexel", argc, argv);
}
