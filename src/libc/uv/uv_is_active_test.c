// Copyright (c) 2017 Nuxi, https://nuxi.nl/
//
// This file is distributed under a 2-clause BSD license.
// See the LICENSE file for details.

#include <stdbool.h>
#include <testing.h>
#include <uv.h>

static void close_noop(uv_handle_t *handle) {
}

static void async_never_called(uv_async_t *handle) {
  ASSERT_TRUE(false);
}

TEST(uv_is_active, async) {
  uv_loop_t loop;
  ASSERT_EQ(0, uv_loop_init(&loop));

  // Async object should be active right after creation.
  uv_async_t async;
  ASSERT_EQ(0, uv_async_init(&loop, &async, async_never_called));
  ASSERT_TRUE(uv_is_active((uv_handle_t *)&async));

  // Closing the async object should deactivate it.
  uv_close((uv_handle_t *)&async, close_noop);
  ASSERT_FALSE(uv_is_active((uv_handle_t *)&async));

  ASSERT_EQ(0, uv_run(&loop, UV_RUN_DEFAULT));
  ASSERT_EQ(0, uv_loop_close(&loop));
}

static void check_never_called(uv_check_t *handle) {
  ASSERT_TRUE(false);
}

TEST(uv_is_active, check) {
  uv_loop_t loop;
  ASSERT_EQ(0, uv_loop_init(&loop));

  // Check objects should not be active upon creation.
  uv_check_t check;
  ASSERT_EQ(0, uv_check_init(&loop, &check));
  ASSERT_FALSE(uv_is_active((uv_handle_t *)&check));

  // Starting the check will make it active.
  ASSERT_EQ(0, uv_check_start(&check, check_never_called));
  ASSERT_TRUE(uv_is_active((uv_handle_t *)&check));

  // Stopping the check will make it inactive again.
  ASSERT_EQ(0, uv_check_stop(&check));
  ASSERT_FALSE(uv_is_active((uv_handle_t *)&check));

  // Closing the check, even when active, should deactivate it.
  ASSERT_EQ(0, uv_check_start(&check, check_never_called));
  uv_close((uv_handle_t *)&check, close_noop);
  ASSERT_FALSE(uv_is_active((uv_handle_t *)&check));

  ASSERT_EQ(0, uv_run(&loop, UV_RUN_DEFAULT));
  ASSERT_EQ(0, uv_loop_close(&loop));
}

static void idle_never_called(uv_idle_t *handle) {
  ASSERT_TRUE(false);
}

TEST(uv_is_active, idle) {
  uv_loop_t loop;
  ASSERT_EQ(0, uv_loop_init(&loop));

  // Idle objects should not be active upon creation.
  uv_idle_t idle;
  ASSERT_EQ(0, uv_idle_init(&loop, &idle));
  ASSERT_FALSE(uv_is_active((uv_handle_t *)&idle));

  // Starting the idle will make it active.
  ASSERT_EQ(0, uv_idle_start(&idle, idle_never_called));
  ASSERT_TRUE(uv_is_active((uv_handle_t *)&idle));

  // Stopping the idle will make it inactive again.
  ASSERT_EQ(0, uv_idle_stop(&idle));
  ASSERT_FALSE(uv_is_active((uv_handle_t *)&idle));

  // Closing the idle, even when active, should deactivate it.
  ASSERT_EQ(0, uv_idle_start(&idle, idle_never_called));
  uv_close((uv_handle_t *)&idle, close_noop);
  ASSERT_FALSE(uv_is_active((uv_handle_t *)&idle));

  ASSERT_EQ(0, uv_run(&loop, UV_RUN_DEFAULT));
  ASSERT_EQ(0, uv_loop_close(&loop));
}

// TODO(ed): Add tests for uv_pipe_t!
// TODO(ed): Add tests for uv_poll_t!

static void prepare_never_called(uv_prepare_t *handle) {
  ASSERT_TRUE(false);
}

TEST(uv_is_active, prepare) {
  uv_loop_t loop;
  ASSERT_EQ(0, uv_loop_init(&loop));

  // Prepare objects should not be active upon creation.
  uv_prepare_t prepare;
  ASSERT_EQ(0, uv_prepare_init(&loop, &prepare));
  ASSERT_FALSE(uv_is_active((uv_handle_t *)&prepare));

  // Starting the prepare will make it active.
  ASSERT_EQ(0, uv_prepare_start(&prepare, prepare_never_called));
  ASSERT_TRUE(uv_is_active((uv_handle_t *)&prepare));

  // Stopping the prepare will make it inactive again.
  ASSERT_EQ(0, uv_prepare_stop(&prepare));
  ASSERT_FALSE(uv_is_active((uv_handle_t *)&prepare));

  // Closing the prepare, even when active, should deactivate it.
  ASSERT_EQ(0, uv_prepare_start(&prepare, prepare_never_called));
  uv_close((uv_handle_t *)&prepare, close_noop);
  ASSERT_FALSE(uv_is_active((uv_handle_t *)&prepare));

  ASSERT_EQ(0, uv_run(&loop, UV_RUN_DEFAULT));
  ASSERT_EQ(0, uv_loop_close(&loop));
}

// TODO(ed): Add tests for uv_process_t!
// TODO(ed): Add tests for uv_stream_t!
// TODO(ed): Add tests for uv_tcp_t!
// TODO(ed): Add tests for uv_timer_t!