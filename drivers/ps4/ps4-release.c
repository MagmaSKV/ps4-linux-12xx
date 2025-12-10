#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/utsname.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/string.h>
#include <linux/seq_file.h>
#include <linux/slab.h>
#include <asm/ps4.h>
#include <linux/delay.h>
#include <linux/kthread.h>

extern unsigned long ps4_tsc_freq_hz;

static unsigned long read_vram_gb(void)
{
    struct file *f;
    char buf[32];
    loff_t pos = 0;
    unsigned long vram_bytes = 0;
    ssize_t ret;

    f = filp_open("/sys/class/drm/card0/device/mem_info_vram_total", O_RDONLY, 0);
    if (IS_ERR(f))
        return 0;

    ret = kernel_read(f, buf, sizeof(buf) - 1, &pos);
    if (ret < 0) {
        filp_close(f, NULL);
        return 0;
    }
    buf[ret] = '\0';

    if (kstrtoul(buf, 10, &vram_bytes))
        vram_bytes = 0;

    filp_close(f, NULL);

    unsigned long vram_gb = (vram_bytes * 10 + (1UL << 29)) >> 30;
    return vram_gb;
}

static unsigned long read_cpu_ghz(void)
{
    unsigned long hz = ps4_tsc_freq_hz;
    unsigned long mhz = (hz + 500000) / 1000000;

    unsigned long ghz_int = mhz / 1000;
    unsigned long ghz_frac = ((mhz % 1000) + 50) / 100;

    if (ghz_frac == 10) { //X.95 -> X+1.0
        ghz_int += 1;
        ghz_frac = 0;
    }
	
    return ghz_int * 10 + ghz_frac;
}

static int ps4_release_thread(void *arg)
{
    unsigned long vram_gb = 0, cpu_ghz = 0;
    struct new_utsname *u = utsname();
    char buf[128];
    char new_version[256];

    msleep(5000);

    vram_gb = read_vram_gb();
    cpu_ghz = read_cpu_ghz();

    // Branding visual limpio
    snprintf(buf, sizeof(buf), "(%lu.%luGB VRAM) (%lu.%luGHz)",
             vram_gb / 10, vram_gb % 10,
             cpu_ghz / 10, cpu_ghz % 10);

    // Copiamos versión original
    strncpy(new_version, u->version, sizeof(new_version) - 1);
    new_version[sizeof(new_version) - 1] = '\0';

    // Concatenamos branding
    strlcat(new_version, " ", sizeof(new_version));
    strlcat(new_version, buf, sizeof(new_version));

    // Reemplazamos u->version
    strncpy(u->version, new_version, sizeof(u->version) - 1);
    u->version[sizeof(u->version) - 1] = '\0';

    pr_info("PS4 uname branding added: %s\n", buf);

    return 0;
}

static int __init ps4_release_thread_init(void)
{
    kthread_run(ps4_release_thread, NULL, "ps4_release");
    return 0;
}
late_initcall(ps4_release_thread_init);
