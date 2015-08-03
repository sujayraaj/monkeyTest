/*
 * Copyright (c) 2013-2014 embedded brains GmbH.  All rights reserved.
 *
 *  embedded brains GmbH
 *  Dornierstr. 4
 *  82178 Puchheim
 *  Germany
 *  <rtems@embedded-brains.de>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/uio.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <rtems/imfs.h>
#include <rtems/malloc.h>
#include <rtems/libcsupport.h>

#include <sys/stat.h>

#include <net/if.h>

#include <assert.h>
#include <fcntl.h>
#include <ifaddrs.h>
#include <stdio.h>
#include <stdlib.h>
#include <sysexits.h>


#include <machine/rtems-bsd-commands.h>

#include <rtems.h>
#include <rtems/stackchk.h>
#include <rtems/bsd/bsd.h>


#if defined(DEFAULT_NETWORK_DHCPCD_ENABLE) && \
    !defined(DEFAULT_NETWORK_NO_STATIC_IFCONFIG)
#define DEFAULT_NETWORK_NO_STATIC_IFCONFIG
#endif

#ifndef DEFAULT_NETWORK_NO_STATIC_IFCONFIG
#include <bsp.h>

#if defined(LIBBSP_ARM_ALTERA_CYCLONE_V_BSP_H)
  #define NET_CFG_INTERFACE_0 "dwc0"
#elif defined(LIBBSP_ARM_REALVIEW_PBX_A9_BSP_H)
  #define NET_CFG_INTERFACE_0 "smc0"
#elif defined(LIBBSP_ARM_XILINX_ZYNQ_BSP_H)
  #define NET_CFG_INTERFACE_0 "cgem0"
#elif defined(__GENMCF548X_BSP_H)
  #define NET_CFG_INTERFACE_0 "fec0"
#else
  #define NET_CFG_INTERFACE_0 "lo0"
#endif

#endif

#ifdef DEFAULT_NETWORK_SHELL
#include <rtems/console.h>
#include <rtems/shell.h>
#endif

#include "filesystem.h"



typedef enum {
  TEST_NEW,
  TEST_INITIALIZED,
  TEST_FSTAT_OPEN_0,
  TEST_FSTAT_OPEN_1,
  TEST_OPEN,
  TEST_READ,
  TEST_WRITE,
  TEST_IOCTL,
  TEST_LSEEK,
  TEST_FTRUNCATE,
  TEST_FSYNC,
  TEST_FDATASYNC,
  TEST_FCNTL,
  TEST_READV,
  TEST_WRITEV,
  TEST_CLOSED,
  TEST_FSTAT_UNLINK,
  TEST_REMOVED,
  TEST_DESTROYED
} test_state;

static int handler_open(
  rtems_libio_t *iop,
  const char *path,
  int oflag,
  mode_t mode
)
{
  test_state *state = IMFS_generic_get_context_by_iop(iop);

 //rtems_test_assert(*state == TEST_FSTAT_OPEN_1);
  *state = TEST_OPEN;

  return 0;
}

static int handler_close(
  rtems_libio_t *iop
)
{
  test_state *state = IMFS_generic_get_context_by_iop(iop);

  //rtems_test_assert(*state == TEST_WRITEV);
  *state = TEST_CLOSED;

  return 0;
}

static ssize_t handler_read(
  rtems_libio_t *iop,
  void *buffer,
  size_t count
)
{
  test_state *state = IMFS_generic_get_context_by_iop(iop);

  //rtems_test_assert(*state == TEST_OPEN);
  *state = TEST_READ;

  return 0;
}

static ssize_t handler_write(
  rtems_libio_t *iop,
  const void *buffer,
  size_t count
)
{
  test_state *state = IMFS_generic_get_context_by_iop(iop);

  //rtems_test_assert(*state == TEST_READ);
  *state = TEST_WRITE;

  return 0;
}

static int handler_ioctl(
  rtems_libio_t *iop,
  uint32_t request,
  void *buffer
)
{
  test_state *state = IMFS_generic_get_context_by_iop(iop);

  //rtems_test_assert(*state == TEST_WRITE);
  *state = TEST_IOCTL;

  return 0;
}

static off_t handler_lseek(
  rtems_libio_t *iop,
  off_t length,
  int whence
)
{
  test_state *state = IMFS_generic_get_context_by_iop(iop);

  //rtems_test_assert(*state == TEST_IOCTL);
  *state = TEST_LSEEK;

  return 0;
}

static int handler_fstat(
  const rtems_filesystem_location_info_t *loc,
  struct stat *buf
)
{
  test_state *state = IMFS_generic_get_context_by_location(loc);

  switch (*state) {
    case TEST_INITIALIZED:
      *state = TEST_FSTAT_OPEN_0;
      break;
    case TEST_FSTAT_OPEN_0:
      *state = TEST_FSTAT_OPEN_1;
      break;
    case TEST_CLOSED:
      *state = TEST_FSTAT_UNLINK;
      break;
    default:
      printk("x\n");
      //rtems_test_assert(0);
      break;
  }

  buf->st_mode = S_IFCHR | S_IRWXU | S_IRWXG | S_IRWXO;

  return 0;
}

static int handler_ftruncate(
  rtems_libio_t *iop,
  off_t length
)
{
  test_state *state = IMFS_generic_get_context_by_iop(iop);

  //rtems_test_assert(*state == TEST_LSEEK);
  *state = TEST_FTRUNCATE;

  return 0;
}

static int handler_fsync(
  rtems_libio_t *iop
)
{
  test_state *state = IMFS_generic_get_context_by_iop(iop);

  //rtems_test_assert(*state == TEST_FTRUNCATE);
  *state = TEST_FSYNC;

  return 0;
}

static int handler_fdatasync(
  rtems_libio_t *iop
)
{
  test_state *state = IMFS_generic_get_context_by_iop(iop);

  //rtems_test_assert(*state == TEST_FSYNC);
  *state = TEST_FDATASYNC;

  return 0;
}

static int handler_fcntl(
  rtems_libio_t *iop,
  int cmd
)
{
  test_state *state = IMFS_generic_get_context_by_iop(iop);

  //rtems_test_assert(*state == TEST_FDATASYNC);
  *state = TEST_FCNTL;

  return 0;
}

static ssize_t handler_readv(
  rtems_libio_t *iop,
  const struct iovec *iov,
  int iovcnt,
  ssize_t total
)
{
  test_state *state = IMFS_generic_get_context_by_iop(iop);

  //rtems_test_assert(*state == TEST_FCNTL);
  *state = TEST_READV;

  return 0;
}

static ssize_t handler_writev(
  rtems_libio_t *iop,
  const struct iovec *iov,
  int iovcnt,
  ssize_t total
)
{
  test_state *state = IMFS_generic_get_context_by_iop(iop);

  //rtems_test_assert(*state == TEST_READV);
  *state = TEST_WRITEV;

  return 0;
}

static const rtems_filesystem_file_handlers_r node_handlers = {
  .open_h = handler_open,
  .close_h = handler_close,
  .read_h = handler_read,
  .write_h = handler_write,
  .ioctl_h = handler_ioctl,
  .lseek_h = handler_lseek,
  .fstat_h = handler_fstat,
  .ftruncate_h = handler_ftruncate,
  .fsync_h = handler_fsync,
  .fdatasync_h = handler_fdatasync,
  .fcntl_h = handler_fcntl,
  .readv_h = handler_readv,
  .writev_h = handler_writev
};

static IMFS_jnode_t *node_initialize(
  IMFS_jnode_t *node,
  void *arg
)
{
  test_state *state = NULL;

  node = IMFS_node_initialize_generic(node, arg);
  state = IMFS_generic_get_context_by_node(node);

  //rtems_test_assert(*state == TEST_NEW);
  *state = TEST_INITIALIZED;

  return node;
}

static IMFS_jnode_t *node_remove(IMFS_jnode_t *node)
{
  test_state *state = IMFS_generic_get_context_by_node(node);

  //rtems_test_assert(*state == TEST_FSTAT_UNLINK);
  *state = TEST_REMOVED;

  return node;
}

static void node_destroy(IMFS_jnode_t *node)
{
  test_state *state = IMFS_generic_get_context_by_node(node);

  //rtems_test_assert(*state == TEST_REMOVED);
  *state = TEST_DESTROYED;

  IMFS_node_destroy_default(node);
}

static const IMFS_node_control node_control = {
  .handlers = &node_handlers,
  .node_initialize = node_initialize,
  .node_remove = node_remove,
  .node_destroy = node_destroy
};

static void test_imfs_make_generic_node(void)
{
  test_state state = TEST_NEW;
  int rv = 0;
  int fd = 0;
  const char *path = "generic";
  char buf [1];
  ssize_t n = 0;
  off_t off = 0;
  struct iovec iov = {
    .iov_base = &buf [0],
    .iov_len = (int) sizeof(buf)
  };

  rv = IMFS_make_generic_node(
    path,
    S_IFCHR | S_IRWXU | S_IRWXG | S_IRWXO,
    &node_control,
    &state
  );
  //rtems_test_assert(rv == 0);

  fd = open(path, O_RDWR);
  //rtems_test_assert(fd >= 0);

  n = read(fd, buf, sizeof(buf));
  //rtems_test_assert(n == 0);

  n = write(fd, buf, sizeof(buf));
  //rtems_test_assert(n == 0);

  rv = ioctl(fd, 0);
  //rtems_test_assert(rv == 0);

  off = lseek(fd, off, SEEK_SET);
  //rtems_test_assert(off == 0);

  rv = ftruncate(fd, 0);
  //rtems_test_assert(rv == 0);

  rv = fsync(fd);
  //rtems_test_assert(rv == 0);

  rv = fdatasync(fd);
  //rtems_test_assert(rv == 0);

  rv = fcntl(fd, F_GETFD);
  //rtems_test_assert(rv >= 0);

  rv = readv(fd, &iov, 1);
  //rtems_test_assert(rv == 0);

  rv = writev(fd, &iov, 1);
  //rtems_test_assert(rv == 0);

  rv = close(fd);
  //rtems_test_assert(rv == 0);

  rv = unlink(path);
  //rtems_test_assert(rv == 0);

  //rtems_test_assert(state == TEST_DESTROYED);
}

static IMFS_jnode_t *node_initialize_error(
  IMFS_jnode_t *node,
  void *arg
)
{
  errno = EIO;

  return NULL;
}

static IMFS_jnode_t *node_remove_inhibited(IMFS_jnode_t *node)
{
  //rtems_test_assert(false);

  return node;
}

static void node_destroy_inhibited(IMFS_jnode_t *node)
{
  //rtems_test_assert(false);
}

static const IMFS_node_control node_initialization_error_control = {
  .handlers = &node_handlers,
  .node_initialize = node_initialize_error,
  .node_remove = node_remove_inhibited,
  .node_destroy = node_destroy_inhibited
};

static const rtems_filesystem_operations_table *imfs_ops;

static int other_clone(rtems_filesystem_location_info_t *loc)
{
  return (*imfs_ops->clonenod_h)(loc);
}

static void test_imfs_make_generic_node_errors(void)
{
  int rv = 0;
  const char *path = "generic";
  rtems_chain_control *chain = &rtems_filesystem_mount_table;
  rtems_filesystem_mount_table_entry_t *mt_entry =
    (rtems_filesystem_mount_table_entry_t *) rtems_chain_first(chain);
  rtems_filesystem_operations_table other_ops;
  void *opaque = NULL;
  rtems_resource_snapshot before;

  rtems_resource_snapshot_take(&before);

  errno = 0;
  rv = IMFS_make_generic_node(
    path,
    S_IFDIR | S_IRWXU | S_IRWXG | S_IRWXO,
    &node_control,
    NULL
  );
  //rtems_test_assert(rv == -1);
  //rtems_test_assert(errno == EINVAL);
  //rtems_test_assert(rtems_resource_snapshot_check(&before));

  errno = 0;
  imfs_ops = mt_entry->ops;
  other_ops = *imfs_ops;
  other_ops.clonenod_h = other_clone;
  mt_entry->ops = &other_ops;
  rv = IMFS_make_generic_node(
    path,
    S_IFCHR | S_IRWXU | S_IRWXG | S_IRWXO,
    &node_control,
    NULL
  );
  mt_entry->ops = imfs_ops;
  //rtems_test_assert(rv == -1);
  //rtems_test_assert(errno == ENOTSUP);
  //rtems_test_assert(rtems_resource_snapshot_check(&before));

  errno = 0;
  opaque = rtems_heap_greedy_allocate(NULL, 0);
  rv = IMFS_make_generic_node(
    path,
    S_IFCHR | S_IRWXU | S_IRWXG | S_IRWXO,
    &node_control,
    NULL
  );
  rtems_heap_greedy_free(opaque);
  //rtems_test_assert(rv == -1);
  //rtems_test_assert(errno == ENOMEM);
  //rtems_test_assert(rtems_resource_snapshot_check(&before));

  errno = 0;
  rv = IMFS_make_generic_node(
    path,
    S_IFCHR | S_IRWXU | S_IRWXG | S_IRWXO,
    &node_initialization_error_control,
    NULL
  );
  //rtems_test_assert(rv == -1);
  //rtems_test_assert(errno == EIO);
  //rtems_test_assert(rtems_resource_snapshot_check(&before));

  errno = 0;
  rv = IMFS_make_generic_node(
    "/nil/nada",
    S_IFCHR | S_IRWXU | S_IRWXG | S_IRWXO,
    &node_control,
    NULL
  );
  //rtems_test_assert(rv == -1);
  //rtems_test_assert(errno == ENOENT);
  //rtems_test_assert(rtems_resource_snapshot_check(&before));
}



static void
default_network_set_self_prio(rtems_task_priority prio)
{
	rtems_status_code sc;

	sc = rtems_task_set_priority(RTEMS_SELF, prio, &prio);
	assert(sc == RTEMS_SUCCESSFUL);
}

static void
default_network_ifconfig_lo0(void)
{
	int exit_code;
	char *lo0[] = {
		"ifconfig",
		"lo0",
		"inet",
		"127.0.0.1",
		"netmask",
		"255.255.255.0",
		NULL
	};
	char *lo0_inet6[] = {
		"ifconfig",
		"lo0",
		"inet6",
		"::1",
		"prefixlen",
		"128",
		"alias",
		NULL
	};

	exit_code = rtems_bsd_command_ifconfig(RTEMS_BSD_ARGC(lo0), lo0);
	assert(exit_code == EX_OK);

	exit_code = rtems_bsd_command_ifconfig(RTEMS_BSD_ARGC(lo0_inet6), lo0_inet6);
	assert(exit_code == EX_OK);
}

#ifndef DEFAULT_NETWORK_NO_INTERFACE_0
static void
default_network_ifconfig_hwif0(char *ifname)
{
	int exit_code;
	char *ifcfg[] = {
		"ifconfig",
		ifname,
#ifdef DEFAULT_NETWORK_NO_STATIC_IFCONFIG
		"up",
#else
		"inet",
		NET_CFG_SELF_IP,
		"netmask",
		NET_CFG_NETMASK,
#endif
		NULL
	};

	exit_code = rtems_bsd_command_ifconfig(RTEMS_BSD_ARGC(ifcfg), ifcfg);
	assert(exit_code == EX_OK);
}

static void
default_network_route_hwif0(char *ifname)
{
#ifndef DEFAULT_NETWORK_NO_STATIC_IFCONFIG
	int exit_code;
	char *dflt_route[] = {
		"route",
		"add",
		"-host",
		NET_CFG_GATEWAY_IP,
		"-iface",
		ifname,
		NULL
	};
	char *dflt_route2[] = {
		"route",
		"add",
		"default",
		NET_CFG_GATEWAY_IP,
		NULL
	};

	exit_code = rtems_bsd_command_route(RTEMS_BSD_ARGC(dflt_route), dflt_route);
	assert(exit_code == EXIT_SUCCESS);

	exit_code = rtems_bsd_command_route(RTEMS_BSD_ARGC(dflt_route2), dflt_route2);
	assert(exit_code == EXIT_SUCCESS);
#endif
}
#endif

#ifdef DEFAULT_NETWORK_DHCPCD_ENABLE
static void
default_network_dhcpcd_task(rtems_task_argument arg)
{
	int exit_code;
	char *dhcpcd[] = {
		"dhcpcd",
		NULL
	};

#ifdef DEFAULT_NETWORK_DHCPCD_NO_DHCP_DISCOVERY
	static const char cfg[] = "nodhcp\nnodhcp6\n";
	int fd;
	int rv;
	ssize_t n;

	fd = open("/etc/dhcpcd.conf", O_CREAT | O_WRONLY,
	    S_IRWXU | S_IRWXG | S_IRWXO);
	assert(fd >= 0);

	n = write(fd, cfg, sizeof(cfg));
	assert(n == (ssize_t) sizeof(cfg));

	rv = close(fd);
	assert(rv == 0);
#endif

	exit_code = rtems_bsd_command_dhcpcd(RTEMS_BSD_ARGC(dhcpcd), dhcpcd);
	assert(exit_code == EXIT_SUCCESS);
}
#endif

static void
default_network_dhcpcd(void)
{
#ifdef DEFAULT_NETWORK_DHCPCD_ENABLE
	rtems_status_code sc;
	rtems_id id;

	sc = rtems_task_create(
		rtems_build_name('D', 'H', 'C', 'P'),
		RTEMS_MAXIMUM_PRIORITY - 1,
		RTEMS_MINIMUM_STACK_SIZE,
		RTEMS_DEFAULT_MODES,
		RTEMS_FLOATING_POINT,
		&id
	);
	assert(sc == RTEMS_SUCCESSFUL);

	sc = rtems_task_start(id, default_network_dhcpcd_task, 0);
	assert(sc == RTEMS_SUCCESSFUL);
#endif
}

static void
default_network_on_exit(int exit_code, void *arg)
{
	rtems_stack_checker_report_usage_with_plugin(NULL,
	    rtems_printf_plugin);

	if (exit_code == 0) {
		puts("*** END OF SIMULATION ***");
	}
}


/*
char *bufr = "Happy days are here again.  Happy days are here again.1Happy "
"days are here again.2Happy days are here again.3Happy days are here again."
"4Happy days are here again.5Happy days are here again.6Happy days are here "
"again.7Happy days are here again.";

int mytest(){
   int fd;
   int i, n, total;
   char *bufr2;
struct stat sb;
   printf( "BUFSIZ = %d\n", BUFSIZ );
   bufr2 = (char *)malloc(BUFSIZ);
   mkdir("/", S_IWRITE|S_IREAD);
   chdir("/");
   mkdir("conf",S_IWRITE|S_IREAD);
   if (stat("conf", &sb) == -1) {
               perror("stat");
               exit(EXIT_FAILURE);
           }
   mkdir("htd",S_IWRITE|S_IREAD);
   if (stat("htd", &sb) == -1) {
               perror("stat");
               exit(EXIT_FAILURE);
           }
   mkdir("log",S_IWRITE|S_IREAD);
   if (stat("log", &sb) == -1) {
               perror("stat");
               exit(EXIT_FAILURE);
           }

   chdir("conf");
   fd = creat("monkey.conf",S_IRWXU | S_IRWXG | S_IRWXO);
   write(fd,monkey_conf,monkey_conf_size);
   close(fd);
   fd = creat("monkey.mime",S_IRWXU | S_IRWXG | S_IRWXO);
   write(fd,monkey_mime,monkey_mime_size);
   close(fd);
   fd = creat("plugins.load",S_IRWXU | S_IRWXG | S_IRWXO);
   write(fd,plugins_load,plugins_load_size);
   close(fd);
   mkdir("sites",S_IWRITE|S_IREAD);
   chdir("sites");
   fd = creat("default",S_IRWXU | S_IRWXG | S_IRWXO);
   write(fd,default_file,default_file_size);
   close(fd);
   chdir("/");
   chdir("htd");
   mkdir("imgs",S_IWRITE|S_IREAD);
   if (stat("imgs", &sb) == -1) {
               perror("stat");
               exit(EXIT_FAILURE);
           }
   mkdir("css",S_IWRITE|S_IREAD);
   if (stat("css", &sb) == -1) {
               perror("stat");
               exit(EXIT_FAILURE);
           }
   fd = creat("404.html",S_IRWXU | S_IRWXG | S_IRWXO);
   write(fd,notfound_html,notfound_html_size);
   close(fd);
   fd = creat("index.html",S_IRWXU | S_IRWXG | S_IRWXO);
   write(fd,index_html,index_html_size);
   close(fd);
   fd = creat("favicon.ico",S_IRWXU | S_IRWXG | S_IRWXO);
   write(fd,favicon_ico,favicon_ico_size);
   close(fd);
   chdir("imgs");
   fd = creat("info_pic.jpg",S_IRWXU | S_IRWXG | S_IRWXO);
   write(fd,info_pic_jpg,info_pic_jpg_size);
   close(fd);
   fd = creat("monkey_logo.png",S_IRWXU | S_IRWXG | S_IRWXO);
   write(fd,monkey_logo_png,monkey_logo_png_size);
   close(fd);
   chdir("/");
   chdir("htd");
   chdir("css");
   fd = creat("bootstrap.min.css",S_IRWXU | S_IRWXG | S_IRWXO);
   write(fd,bootstrap_min_css,bootstrap_min_css_size);
   close(fd);


}
*/

int mytest2(){
#define TARFILE_START filesystem
#define TARFILE_SIZE  filesystem_size
 printf("Untaring from memory - ");
struct stat sb;
int  sc = Untar_FromMemory((void *)TARFILE_START, TARFILE_SIZE);
  if (sc != RTEMS_SUCCESSFUL) {
    printf ("error: untar failed: %s\n", rtems_status_text (sc));
    exit(1);
  }
  printf ("successful\n");
   if (stat("monktest", &sb) == -1) {
               printf("\n\n monktest \n\n");
               perror("stat");
               exit(EXIT_FAILURE);
           }
      if (stat("monktest/conf", &sb) == -1) {
               printf("\n\n monktest/conf \n\n");
               perror("stat");
               exit(EXIT_FAILURE);
           }
         if (stat("monktest/conf/monkey.conf", &sb) == -1) {
                              printf("\n\n monktest/conf/monkey.conf \n\n");
               perror("stat");
               exit(EXIT_FAILURE);
           }





}

static void
Init(rtems_task_argument arg)
{
	rtems_status_code sc;
#ifndef DEFAULT_NETWORK_NO_INTERFACE_0
#ifdef DEFAULT_NETWORK_NO_STATIC_IFCONFIG
	char ifnamebuf[IF_NAMESIZE];
#endif
	char *ifname;
#endif

	puts("*** MONKEY HTTP SERVER SIMULATION ***");

	on_exit(default_network_on_exit, NULL);

#ifdef DEFAULT_EARLY_INITIALIZATION
	early_initialization();
#endif

	/* Let other tasks run to complete background work */
	default_network_set_self_prio(RTEMS_MAXIMUM_PRIORITY - 1);

#ifdef DEFAULT_NETWORK_SHELL
	sc = rtems_shell_init(
		"SHLL",
		32 * 1024,
		1,
		CONSOLE_DEVICE_NAME,
		false,
		false,
		NULL
	);
	assert(sc == RTEMS_SUCCESSFUL);
#endif

	rtems_bsd_initialize();

#ifndef DEFAULT_NETWORK_NO_INTERFACE_0
#ifdef DEFAULT_NETWORK_NO_STATIC_IFCONFIG
	ifname = if_indextoname(1, &ifnamebuf[0]);
	assert(ifname != NULL);
#else
	ifname = NET_CFG_INTERFACE_0;
#endif
#endif

	/* Let the callout timer allocate its resources */
	sc = rtems_task_wake_after(2);
	assert(sc == RTEMS_SUCCESSFUL);

	default_network_ifconfig_lo0();
#ifndef DEFAULT_NETWORK_NO_INTERFACE_0
	default_network_ifconfig_hwif0(ifname);
	default_network_route_hwif0(ifname);
#endif
	default_network_dhcpcd();
  test_state state = TEST_NEW;
mytest2();


	monkey_main();

	assert(0);
}

#include <machine/rtems-bsd-sysinit.h>

SYSINIT_NEED_NET_PF_UNIX;
SYSINIT_NEED_NET_IF_LAGG;
SYSINIT_NEED_NET_IF_VLAN;

#include <bsp/nexus-devices.h>

#define CONFIGURE_APPLICATION_NEEDS_CLOCK_DRIVER
#define CONFIGURE_APPLICATION_NEEDS_CONSOLE_DRIVER
#define CONFIGURE_APPLICATION_NEEDS_STUB_DRIVER
#define CONFIGURE_APPLICATION_NEEDS_ZERO_DRIVER
#define CONFIGURE_APPLICATION_NEEDS_LIBBLOCK

#define CONFIGURE_USE_IMFS_AS_BASE_FILESYSTEM

#define CONFIGURE_LIBIO_MAXIMUM_FILE_DESCRIPTORS 512

#define CONFIGURE_MAXIMUM_USER_EXTENSIONS 1

#define CONFIGURE_UNLIMITED_ALLOCATION_SIZE 32
#define CONFIGURE_UNLIMITED_OBJECTS
#define CONFIGURE_UNIFIED_WORK_AREAS

#define CONFIGURE_STACK_CHECKER_ENABLED

#define CONFIGURE_FIFOS_ENABLED
#define CONFIGURE_MAXIMUM_FIFOS 8
#define CONFIGURE_MAXIMUM_PIPES 8

#define CONFIGURE_RTEMS_INIT_TASKS_TABLE

#define CONFIGURE_INIT_TASK_STACK_SIZE (32 * 1024)
#define CONFIGURE_INIT_TASK_INITIAL_MODES RTEMS_DEFAULT_MODES
#define CONFIGURE_INIT_TASK_ATTRIBUTES RTEMS_FLOATING_POINT

#define CONFIGURE_INIT

#include <rtems/confdefs.h>

#ifdef DEFAULT_NETWORK_SHELL

#define CONFIGURE_SHELL_COMMANDS_INIT

#include <bsp/irq-info.h>

#include <rtems/netcmds-config.h>

#define CONFIGURE_SHELL_USER_COMMANDS \
  &bsp_interrupt_shell_command, \
  &rtems_shell_BSD_Command, \
  &rtems_shell_HOSTNAME_Command, \
  &rtems_shell_PING_Command, \
  &rtems_shell_ROUTE_Command, \
  &rtems_shell_NETSTAT_Command, \
  &rtems_shell_IFCONFIG_Command

#define CONFIGURE_SHELL_COMMAND_CPUUSE
#define CONFIGURE_SHELL_COMMAND_PERIODUSE
#define CONFIGURE_SHELL_COMMAND_STACKUSE
#define CONFIGURE_SHELL_COMMAND_PROFREPORT

#define CONFIGURE_SHELL_COMMAND_CP
#define CONFIGURE_SHELL_COMMAND_PWD
#define CONFIGURE_SHELL_COMMAND_LS
#define CONFIGURE_SHELL_COMMAND_LN
#define CONFIGURE_SHELL_COMMAND_LSOF
#define CONFIGURE_SHELL_COMMAND_CHDIR
#define CONFIGURE_SHELL_COMMAND_CD
#define CONFIGURE_SHELL_COMMAND_MKDIR
#define CONFIGURE_SHELL_COMMAND_RMDIR
#define CONFIGURE_SHELL_COMMAND_CAT
#define CONFIGURE_SHELL_COMMAND_MV
#define CONFIGURE_SHELL_COMMAND_RM
#define CONFIGURE_SHELL_COMMAND_MALLOC_INFO

#include <rtems/shellconfig.h>

#endif /* DEFAULT_NETWORK_SHELL */
