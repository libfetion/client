#include "support.h"

#include "csync_update.c"

CSYNC *csync;

static void setup(void) {
  fail_if(system("mkdir -p /tmp/check_csync") < 0, "Setup failed");
  fail_if(system("mkdir -p /tmp/check_csync1") < 0, "Setup failed");
  fail_if(system("mkdir -p /tmp/check_csync2") < 0, "Setup failed");
  fail_if(csync_create(&csync, "/tmp/check_csync1", "/tmp/check_csync2") < 0, "Setup failed");
  fail_if(csync_set_config_dir(csync, "/tmp/check_csync") < 0, "Setup failed");
  fail_if(csync_init(csync) < 0, NULL);
}

static void setup_ftw(void) {
  fail_if(system("mkdir -p /tmp/check_csync") < 0, "Setup failed");
  fail_if(system("mkdir -p /tmp/check_csync1") < 0, "Setup failed");
  fail_if(system("mkdir -p /tmp/check_csync2") < 0, "Setup failed");
  fail_if(csync_create(&csync, "/tmp", "/tmp") < 0, "Setup failed");
  fail_if(csync_set_config_dir(csync, "/tmp/check_csync") < 0, "Setup failed");
  fail_if(csync_init(csync) < 0, NULL);
}

static void teardown(void) {
  fail_if(csync_destroy(csync) < 0, "Teardown failed");
  unsetenv("CSYNC_NOMEMORY");
}

static void teardown_rm(void) {
  teardown();
  fail_if(system("rm -rf /tmp/check_csync") < 0, "Teardown failed");
  fail_if(system("rm -rf /tmp/check_csync1") < 0, "Teardown failed");
  fail_if(system("rm -rf /tmp/check_csync2") < 0, "Teardown failed");
}

static void stat_init(csync_vio_file_stat_t *fs) {
  time_t t;

  fail_if(fs == NULL, NULL);

  fs->name = c_strdup("file.txt");
  fs->fields = CSYNC_VIO_FILE_STAT_FIELDS_NONE;

  fs->type = CSYNC_VIO_FILE_TYPE_REGULAR;
  fs->mode = 0777;
  fs->inode = 619070;

  fs->device = 0;
  fs->size = 157459;
  fs->nlink = 1;

  fs->uid = 1000;
  fs->gid = 1000;

  fs->blksize = 312;
  fs->blkcount = 4096;

  fs->atime = fs->mtime = fs->ctime = time(&t);
}

static int failing_fn(CSYNC *ctx,
    const char *file,
    const csync_vio_file_stat_t *fs,
    enum csync_ftw_flags_e flag) {
  (void) ctx;
  (void) file;
  (void) fs;
  (void) flag;

  return -1;
}

START_TEST (check_csync_detect_update)
{
  csync_file_stat_t *st = NULL;
  csync_vio_file_stat_t *fs = NULL;

  fs = csync_vio_file_stat_new();
  stat_init(fs);
  fs->mtime = 1217597845;

  fail_unless(_csync_detect_update(csync, "/tmp/check_csync1/file.txt", fs, CSYNC_FTW_TYPE_FILE) == 0, NULL);

  /* the instruction should be set to new  */
  st = c_rbtree_node_data(csync->local.tree->root);
  fail_unless(st->instruction == CSYNC_INSTRUCTION_NEW, "instruction is %s", csync_instruction_str(st->instruction));

  /* set the instruction to UPDATED that it gets written to the statedb */
  st->instruction = CSYNC_INSTRUCTION_UPDATED;

  /* create a statedb */
  csync_set_status(csync, 0xFFFF);

  csync_vio_file_stat_destroy(fs);
}
END_TEST

START_TEST (check_csync_detect_update_db_none)
{
  csync_file_stat_t *st = NULL;
  csync_vio_file_stat_t *fs = NULL;

  fs = csync_vio_file_stat_new();
  stat_init(fs);
  fs->mtime = 1217597845;

  fail_unless(_csync_detect_update(csync, "/tmp/check_csync1/file.txt", fs, CSYNC_FTW_TYPE_FILE) == 0, NULL);

  /* the instruction should be set to new  */
  st = c_rbtree_node_data(csync->local.tree->root);
  fail_unless(st->instruction == CSYNC_INSTRUCTION_NONE, "instruction is %s", csync_instruction_str(st->instruction));

  /* set the instruction to UPDATED that it gets written to the statedb */
  st->instruction = CSYNC_INSTRUCTION_UPDATED;

  /* create a statedb */
  csync_set_status(csync, 0xFFFF);

  csync_vio_file_stat_destroy(fs);
}
END_TEST

START_TEST (check_csync_detect_update_db_eval)
{
  csync_file_stat_t *st = NULL;
  csync_vio_file_stat_t *fs = NULL;

  fs = csync_vio_file_stat_new();
  stat_init(fs);

  fail_unless(_csync_detect_update(csync, "/tmp/check_csync1/file.txt", fs, CSYNC_FTW_TYPE_FILE) == 0, NULL);

  /* the instruction should be set to new  */
  st = c_rbtree_node_data(csync->local.tree->root);
  fail_unless(st->instruction == CSYNC_INSTRUCTION_EVAL, "instruction is %s", csync_instruction_str(st->instruction));

  /* set the instruction to UPDATED that it gets written to the statedb */
  st->instruction = CSYNC_INSTRUCTION_UPDATED;

  /* create a statedb */
  csync_set_status(csync, 0xFFFF);

  csync_vio_file_stat_destroy(fs);
}
END_TEST

START_TEST (check_csync_detect_update_db_rename)
{
  csync_file_stat_t *st = NULL;
  csync_vio_file_stat_t *fs = NULL;

  fs = csync_vio_file_stat_new();

  stat_init(fs);
  free(fs->name);
  fs->name = c_strdup("wurst.txt");

  fail_unless(_csync_detect_update(csync, "/tmp/check_csync1/wurst.txt", fs, CSYNC_FTW_TYPE_FILE) == 0, NULL);

  /* the instruction should be set to rename */
  st = c_rbtree_node_data(csync->local.tree->root);
  fail_unless(st->instruction == CSYNC_INSTRUCTION_RENAME, "instruction is %s", csync_instruction_str(st->instruction));

  /* set the instruction to UPDATED that it gets written to the statedb */
  st->instruction = CSYNC_INSTRUCTION_UPDATED;

  /* create a statedb */
  csync_set_status(csync, 0xFFFF);

  csync_vio_file_stat_destroy(fs);
}
END_TEST

START_TEST (check_csync_detect_update_db_new)
{
  csync_file_stat_t *st = NULL;
  csync_vio_file_stat_t *fs = NULL;

  fs = csync_vio_file_stat_new();
  stat_init(fs);
  fs->inode = 42;

  fail_unless(_csync_detect_update(csync, "/tmp/check_csync1/file.txt", fs, CSYNC_FTW_TYPE_FILE) == 0, NULL);

  /* the instruction should be set to new  */
  st = c_rbtree_node_data(csync->local.tree->root);
  fail_unless(st->instruction == CSYNC_INSTRUCTION_NEW, "instruction is %s", csync_instruction_str(st->instruction));

  /* set the instruction to UPDATED that it gets written to the statedb */
  st->instruction = CSYNC_INSTRUCTION_UPDATED;

  /* create a statedb */
  csync_set_status(csync, 0xFFFF);

  csync_vio_file_stat_destroy(fs);
}
END_TEST

START_TEST (check_csync_detect_update_nlink)
{
  csync_file_stat_t *st = NULL;
  csync_vio_file_stat_t *fs = NULL;

  /* create vio file stat with nlink greater than 1 */
  fs = csync_vio_file_stat_new();
  stat_init(fs);
  fs->nlink = 42;

  /* add it to local tree */
  fail_unless(_csync_detect_update(csync, "/tmp/check_csync1/file.txt", fs, CSYNC_FTW_TYPE_FILE) == 0, NULL);

  /* the instruction should be set to ignore */
  st = c_rbtree_node_data(csync->local.tree->root);
  fail_unless(st->instruction == CSYNC_INSTRUCTION_IGNORE);

  csync_vio_file_stat_destroy(fs);
}
END_TEST

START_TEST (check_csync_detect_update_null)
{
  csync_vio_file_stat_t *fs = NULL;
  fs = csync_vio_file_stat_new();
  stat_init(fs);

  fail_unless(_csync_detect_update(csync, NULL, fs, CSYNC_FTW_TYPE_FILE) < 0, NULL);
  fail_unless(_csync_detect_update(csync, "/tmp/check_csync1/file.txt", NULL, CSYNC_FTW_TYPE_FILE) < 0, NULL);

  csync_vio_file_stat_destroy(fs);
}
END_TEST

START_TEST (check_csync_ftw)
{
  fail_unless(csync_ftw(csync, "/tmp", csync_walker, MAX_DEPTH) == 0, NULL);
}
END_TEST

START_TEST (check_csync_ftw_empty_uri)
{
  fail_unless(csync_ftw(csync, "", csync_walker, MAX_DEPTH) < 0, NULL);
}
END_TEST

START_TEST (check_csync_ftw_failing_fn)
{
  fail_unless(csync_ftw(csync, "/tmp", failing_fn, MAX_DEPTH) < 0, NULL);
}
END_TEST

#ifdef CSYNC_MEM_NULL_TESTS
START_TEST (check_csync_detect_update_no_mem)
{
  csync_vio_file_stat_t *fs = NULL;
  fs = csync_vio_file_stat_new();
  stat_init(fs);

  setenv("CSYNC_NOMEMORY", "1", 1);
  fail_unless(_csync_detect_update(csync, "file.txt", fs, CSYNC_FTW_TYPE_FILE) < 0, NULL);

  csync_vio_file_stat_destroy(fs);
}
END_TEST

START_TEST (check_csync_ftw_nomem)
{
  setenv("CSYNC_NOMEMORY", "1", 1);
  fail_unless(csync_ftw(csync, "/tmp", csync_walker, MAX_DEPTH) < 0, NULL);
}
END_TEST

#endif

static Suite *make_csync_suite(void) {
  Suite *s = suite_create("csync_update");

  create_case_fixture(s, "check_csync_detect_update", check_csync_detect_update, setup, teardown);
  create_case_fixture(s, "check_csync_detect_update_db_none", check_csync_detect_update_db_none, setup, teardown);
  create_case_fixture(s, "check_csync_detect_update_db_eval", check_csync_detect_update_db_eval, setup, teardown);
  create_case_fixture(s, "check_csync_detect_update_db_rename", check_csync_detect_update_db_rename, setup, teardown);
  create_case_fixture(s, "check_csync_detect_update_db_new", check_csync_detect_update_db_new, setup, teardown_rm);
  create_case_fixture(s, "check_csync_detect_update_nlink", check_csync_detect_update_nlink, setup, teardown_rm);
  create_case_fixture(s, "check_csync_detect_update_no_file", check_csync_detect_update_null, setup, teardown_rm);

  create_case_fixture(s, "check_csync_ftw", check_csync_ftw, setup_ftw, teardown_rm);
  create_case_fixture(s, "check_csync_ftw_empty_uri", check_csync_ftw_empty_uri, setup_ftw, teardown_rm);
  create_case_fixture(s, "check_csync_ftw_failing_fn", check_csync_ftw_failing_fn, setup, teardown_rm);

#ifdef CSYNC_MEM_NULL_TESTS
  create_case_fixture(s, "check_csync_ftw_nomem", check_csync_ftw_nomem, setup, teardown_rm);
  create_case_fixture(s, "check_csync_detect_update_nomem", check_csync_detect_update_no_mem, setup, teardown_rm);
#endif
  return s;
}

int main(int argc, char **argv) {
  Suite *s = NULL;
  SRunner *sr = NULL;
  struct argument_s arguments;
  int nf;

  ZERO_STRUCT(arguments);

  cmdline_parse(argc, argv, &arguments);

  s = make_csync_suite();

  sr = srunner_create(s);
  if (arguments.nofork) {
    srunner_set_fork_status(sr, CK_NOFORK);
  }
  srunner_run_all(sr, CK_VERBOSE);
  nf = srunner_ntests_failed(sr);
  srunner_free(sr);

  return (nf == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}

