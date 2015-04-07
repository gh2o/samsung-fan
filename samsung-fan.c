#include <linux/init.h>
#include <linux/module.h>
#include <linux/acpi.h>
#include <linux/platform_device.h>

#define DRIVER_NAME "samsung-fan"
#define WMI_GUID "C16C47BA-50E3-444A-AF3A-B1C348380001"

struct samsung_fan_packet {
	uint16_t magic;
	uint16_t opcode;
	uint8_t reqres;
	uint8_t data[16];
} __packed;

static struct platform_device *samsung_fan_device;

static struct platform_driver samsung_fan_driver = {
	.driver = {
		.name = DRIVER_NAME,
		.owner = THIS_MODULE,
	},
};

static inline bool string_matches(const char *str, const char *kwd) {
	size_t len = strlen(kwd);
	if (strncmp(str, kwd, len) != 0)
		return false;
	return str[len] == '\0' || str[len] == '\n';
}

static int samsung_fan_wmi_call(uint16_t opcode, void *data, size_t len) {
	struct samsung_fan_packet inpkt = {
		.magic = 0x5843,
		.opcode = opcode,
	};
	struct acpi_buffer inbuf = { sizeof(inpkt), &inpkt };
	struct acpi_buffer outbuf = { ACPI_ALLOCATE_BUFFER, NULL };
	union acpi_object *outobj;
	struct samsung_fan_packet *outpkt;
	acpi_status st;
	int ret = -EIO;
	/********/
	if (len > 16) {
		ret = -EINVAL;
		goto err0;
	}
	memcpy(inpkt.data, data, len);
	st = wmi_evaluate_method(WMI_GUID, 1, 0, &inbuf, &outbuf);
	if (ACPI_FAILURE(st)) {
		dev_err(&samsung_fan_device->dev, "Samsung WMI opcode 0x%x failed: %s\n",
				opcode, acpi_format_exception(st));
		goto err0;
	}
	outobj = outbuf.pointer;
	if (outobj->type != ACPI_TYPE_BUFFER) {
		goto err1;
	}
	if (outobj->buffer.length < sizeof(outpkt)) {
		goto err1;
	}
	outpkt = (struct samsung_fan_packet *)outobj->buffer.pointer;
	memcpy(data, outpkt->data, len);
	ret = 0;
err1:
	kfree(outobj);
err0:
	return ret;
}

static int samsung_fan_wmi_call_with_unlock(
		uint16_t unlock_opcode,
		uint16_t opcode, void *data, size_t len) {
	unsigned int unlock_data = 0xAABB;
	int ret;
	/********/
	ret = samsung_fan_wmi_call(unlock_opcode, &unlock_data, sizeof(unlock_data));
	if (ret)
		goto err0;
	if (unlock_data != 0xCCDD) {
		dev_err(&samsung_fan_device->dev, "Samsung WMI incorrect unlock response!\n");
		ret = -EIO;
		goto err0;
	}
	ret = samsung_fan_wmi_call(opcode, data, len);
err0:
	return ret;
}

ssize_t samsung_fan_mode_show(struct device *dev, struct device_attribute *attr,
		char *buf) {
	uint32_t data = 0;
	if (samsung_fan_wmi_call_with_unlock(0x31, 0x31, &data, sizeof(data)))
		return -EIO;
	if (data)
		strcpy(buf, "[auto off] on\n");
	else
		strcpy(buf, "auto off [on]\n");
	return strlen(buf);
}

ssize_t samsung_fan_mode_store(struct device *dev, struct device_attribute *attr,
		const char *buf, size_t count) {
	uint32_t data;
	if (string_matches(buf, "auto"))
		data = 0x810001;
	else if (string_matches(buf, "on"))
		data = 0;
	else if (string_matches(buf, "off"))
		data = 0x800001;
	else
		return -EINVAL;
	if (samsung_fan_wmi_call_with_unlock(0x31, 0x32, &data, sizeof(data)))
		return -EIO;
	else
		return count;
}

static DEVICE_ATTR(mode, S_IRUGO | S_IWUSR, samsung_fan_mode_show, samsung_fan_mode_store);

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
