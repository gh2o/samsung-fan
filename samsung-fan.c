#include <linux/init.h>
#include <linux/module.h>
#include <linux/acpi.h>
#include <linux/platform_device.h>

#define DRIVER_NAME "samsung-fan"
#define WMI_GUID "C16C47BA-50E3-444A-AF3A-B1C348380001"

struct samsung_fan_packet {
	uint16_t magic;
	uint16_t opcode;
	uint32_t value;
	uint8_t padding[24];
};

static inline bool string_matches(const char *str, const char *kwd) {
	size_t len = strlen(kwd);
	if (strncmp(str, kwd, len) != 0)
		return false;
	return str[len] == '\0' || str[len] == '\n';
}

ssize_t samsung_fan_mode_show(struct device *dev, struct device_attribute *attr,
		char *buf) {
	const char *info = "auto on off\n";
	strcpy(buf, info);
	return strlen(info);
}

ssize_t samsung_fan_mode_store(struct device *dev, struct device_attribute *attr,
		const char *buf, size_t count) {
	struct samsung_fan_packet pkt = {
		.magic = 0x5843,
		.opcode = 0x32,
	};
	struct acpi_buffer abuf = { sizeof(pkt), &pkt };
	acpi_status st;
	if (string_matches(buf, "auto"))
		pkt.value = 0x81000100u;
	else if (string_matches(buf, "on"))
		pkt.value = 0;
	else if (string_matches(buf, "off"))
		pkt.value = 0x80000100u;
	else
		return -EINVAL;
	st = wmi_evaluate_method(WMI_GUID, 1, 0, &abuf, &abuf);
	if (ACPI_SUCCESS(st)) {
		return count;
	} else {
		dev_err(dev, "Failed to control fan: %s\n", acpi_format_exception(st));
		return -EIO;
	}
}

static DEVICE_ATTR(mode, S_IRUGO | S_IWUSR, samsung_fan_mode_show, samsung_fan_mode_store);

static struct platform_device *samsung_fan_device;

static struct platform_driver samsung_fan_driver = {
	.driver = {
		.name = DRIVER_NAME,
		.owner = THIS_MODULE,
	},
};

static int samsung_fan_init(void) {
	int ret;
	// check whether method is present
	if (!wmi_has_guid(WMI_GUID))
		return -ENODEV;
	// we're good to go
	ret = platform_driver_register(&samsung_fan_driver);
	if (ret) {
		goto err0;
	}
	samsung_fan_device = platform_device_alloc(DRIVER_NAME, -1);
	if (!samsung_fan_device) {
		ret = -ENOMEM;
		goto err1;
	}
	ret = platform_device_add(samsung_fan_device);
	if (ret) {
		goto err2;
	}
	ret = device_create_file(&samsung_fan_device->dev, &dev_attr_mode);
	if (ret) {
		goto err3;
	}
	return 0;
//err4:
	device_remove_file(&samsung_fan_device->dev, &dev_attr_mode);
err3:
	platform_device_del(samsung_fan_device);
err2:
	platform_device_put(samsung_fan_device);
err1:
	platform_driver_unregister(&samsung_fan_driver);
err0:
	return ret;
}

static void samsung_fan_exit(void) {
	device_remove_file(&samsung_fan_device->dev, &dev_attr_mode);
	platform_device_unregister(samsung_fan_device);
	platform_driver_unregister(&samsung_fan_driver);
}

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Gavin Li");
MODULE_DESCRIPTION("WMI driver for controlling the fan on Samsung laptops.");
MODULE_ALIAS("wmi:" WMI_GUID);
module_init(samsung_fan_init);
module_exit(samsung_fan_exit);
