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

    return vram_bytes >> 30; // Pasar a GB
}

static unsigned long read_cpu_ghz(void)
{
    unsigned long freq_hz = ps4_calibrate_tsc(); // tu función
    return (freq_hz + 500000000) / 1000000000;   // redondear a GHz
}

static int __init ps4_update_release_late(void)
{
    struct new_utsname *u = utsname();
    char buf[65]; // __NEW_UTS_LEN
    unsigned long vram_gb = read_vram_gb();
    unsigned long cpu_ghz = read_cpu_ghz();

    snprintf(buf, sizeof(buf), "SKV-NFT (%luGB VRAM) (%luGHz)", vram_gb, cpu_ghz);

    strlcpy(u->release, buf, sizeof(u->release));

    pr_info("PS4 uname release updated: %s\n", u->release);

    return 0;
}
late_initcall(ps4_update_release_late);
