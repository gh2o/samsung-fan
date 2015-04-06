#include <linux/init.h>
#include <linux/module.h>
#include <linux/acpi.h>

static int samsung_fan_init(void) {
	char inbuf[32] = {0};
	char outbuf[256] = {0};
	struct acpi_buffer ainbuf = { sizeof(inbuf), inbuf };
	struct acpi_buffer aoutbuf = { sizeof(outbuf), outbuf };
	acpi_status st;

	inbuf[0] = 0x43;
	inbuf[1] = 0x58;
	inbuf[2] = 0x32;
	inbuf[5] = 0x01;
	inbuf[7] = 0x80;
	st = wmi_evaluate_method("C16C47BA-50E3-444A-AF3A-B1C348380001",
			1, 0, &ainbuf, &aoutbuf);

	printk("wmimf: status is %u\n", st);
	print_hex_dump(KERN_INFO, "wmisf", DUMP_PREFIX_OFFSET, 16, 1, outbuf, sizeof(outbuf), 1);
	return 0;
}

static void samsung_fan_exit(void) {
}

MODULE_LICENSE("GPL");
module_init(samsung_fan_init);
module_exit(samsung_fan_exit);
