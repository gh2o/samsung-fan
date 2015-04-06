#include <linux/init.h>
#include <linux/module.h>
#include <linux/acpi.h>
#include <linux/platform_device.h>

#define DRIVER_NAME "samsung-fan"

ssize_t samsung_fan_mode_show(struct device *dev, struct device_attribute *attr,
		char *buf) {
	return -ENODEV;
}

ssize_t samsung_fan_mode_store(struct device *dev, struct device_attribute *attr,
		const char *buf, size_t count) {
	return -ENODEV;
}

static int samsung_fan_probe(struct platform_device *pdev) {
	return -ENODEV;
}

static int samsung_fan_remove(struct platform_device *pdev) {
	return -ENODEV;
}

static DEVICE_ATTR(mode, S_IRUGO | S_IWUSR, samsung_fan_mode_show, samsung_fan_mode_store);

static struct platform_device *samsung_fan_device;

static struct platform_driver samsung_fan_driver = {
	.driver = {
		.name = DRIVER_NAME,
		.owner = THIS_MODULE,
	},
	.probe = samsung_fan_probe,
	.remove = samsung_fan_remove
};

static int samsung_fan_init(void) {
	int ret;
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
module_init(samsung_fan_init);
module_exit(samsung_fan_exit);
