#include <linux/device.h>
#include <linux/err.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/string.h>

#include <asm/semaphore.h>

int bus_add_device(struct device * dev);
void bus_remove_device(struct device * dev);


#define to_dev(node) container_of(node,struct device,bus_list)

static struct subsystem devices_subsys;
static struct subsystem bus_subsys;


/**
 *	device_release - free device structure.
 *	@kobj:	device's kobject.
 *
 *	This is called once the reference count for the object
 *	reaches 0. We forward the call to the device's release
 *	method, which should handle actually freeing the structure.
 */
/*
static void device_release(struct device * dev)
{
	if (dev->release)
		dev->release(dev);
	else {
		printk(KERN_ERR "Device '%s' does not have a release() function, "
			"it is broken and must be fixed.\n",
			dev->bus_id);
		WARN_ON(1);
	}
}
*/
/**
 *	device_initialize - init device structure.
 *	@dev:	device.
 *
 *	This prepares the device for use by other layers,
 *	including adding it to the device hierarchy. 
 *	It is the first half of device_register(), if called by
 *	that, though it can also be called separately, so one
 *	may use @dev's fields (e.g. the refcount).
 */

void device_initialize(struct device *dev)
{
	INIT_LIST_HEAD(&dev->node);
	INIT_LIST_HEAD(&dev->children);
	INIT_LIST_HEAD(&dev->driver_list);
	INIT_LIST_HEAD(&dev->bus_list);
	INIT_LIST_HEAD(&dev->dma_pools);
}

/**
 *	device_add - add device to device hierarchy.
 *	@dev:	device.
 *
 *	This is part 2 of device_register(), though may be called 
 *	separately _iff_ device_initialize() has been called separately.
 *
 *	This adds it to the kobject hierarchy via kobject_add(), adds it
 *	to the global and sibling lists for the device, then
 *	adds it to the other relevant subsystems of the driver model.
 */
int device_add(struct device *dev)
{
	int error;

	dev = get_device(dev);
	if (!dev || !strlen(dev->bus_id))
		return -EINVAL;

	pr_debug("DEV: registering device: ID = '%s'\n", dev->bus_id);

	if ((error = bus_add_device(dev)))
		goto BusError;

 Done:
	put_device(dev);
	return error;
 BusError:
	goto Done;
}


/**
 *	device_register - register a device with the system.
 *	@dev:	pointer to the device structure
 *
 *	This happens in two clean steps - initialize the device
 *	and add it to the system. The two steps can be called 
 *	separately, but this is the easiest and most common. 
 *	I.e. you should only call the two helpers separately if 
 *	have a clearly defined need to use and refcount the device
 *	before it is added to the hierarchy.
 */

int device_register(struct device *dev)
{
	device_initialize(dev);
	return device_add(dev);
}


/**
 *	get_device - increment reference count for device.
 *	@dev:	device.
 *
 *	This simply forwards the call to kobject_get(), though
 *	we do take care to provide for the case that we get a NULL
 *	pointer passed in.
 */

struct device * get_device(struct device * dev)
{
// Fixme: we should propably increment a reference count
	return dev;
}


/**
 *	put_device - decrement reference count.
 *	@dev:	device in question.
 */
void put_device(struct device * dev)
{
// Fixme: we should propably decrement a reference counter
}


/**
 *	device_del - delete device from system.
 *	@dev:	device.
 *
 *	This is the first part of the device unregistration 
 *	sequence. This removes the device from the lists we control
 *	from here, has it removed from the other driver model 
 *	subsystems it was added to in device_add(), and removes it
 *	from the kobject hierarchy.
 *
 *	NOTE: this should be called manually _iff_ device_add() was 
 *	also called manually.
 */

void device_del(struct device * dev)
{
	bus_remove_device(dev);
}

/**
 *	device_unregister - unregister device from system.
 *	@dev:	device going away.
 *
 *	We do this in two parts, like we do device_register(). First,
 *	we remove it from all the subsystems with device_del(), then
 *	we decrement the reference count via put_device(). If that
 *	is the final reference count, the device will be cleaned up
 *	via device_release() above. Otherwise, the structure will 
 *	stick around until the final reference to the device is dropped.
 */
void device_unregister(struct device * dev)
{
	pr_debug("DEV: Unregistering device. ID = '%s'\n", dev->bus_id);
	device_del(dev);
	put_device(dev);
}


/**
 *	bus_for_each_dev - device iterator.
 *	@bus:	bus type.
 *	@start:	device to start iterating from.
 *	@data:	data for the callback.
 *	@fn:	function to be called for each device.
 *
 *	Iterate over @bus's list of devices, and call @fn for each,
 *	passing it @data. If @start is not NULL, we use that device to
 *	begin iterating from.
 *
 *	We check the return of @fn each time. If it returns anything
 *	other than 0, we break out and return that value.
 *
 *	NOTE: The device that returns a non-zero value is not retained
 *	in any way, nor is its refcount incremented. If the caller needs
 *	to retain this data, it should do, and increment the reference 
 *	count in the supplied callback.
 */
int bus_for_each_dev(struct bus_type * bus, struct device * start, 
		     void * data, int (*fn)(struct device *, void *))
{
	struct device *dev;
	struct list_head * head;
	int error = 0;

	if (!(bus = get_bus(bus)))
		return -EINVAL;

	head = &bus->devices.list;
	dev = list_prepare_entry(start, head, bus_list);

	down_read(&bus->subsys.rwsem);
	list_for_each_entry_continue(dev, head, bus_list) {
		get_device(dev);
		error = fn(dev,data);
		put_device(dev);
		if (error)
			break;
	}
	up_read(&bus->subsys.rwsem);
	put_bus(bus);
	return error;
}

/**
 *	bus_for_each_drv - driver iterator
 *	@bus:	bus we're dealing with.
 *	@start:	driver to start iterating on.
 *	@data:	data to pass to the callback.
 *	@fn:	function to call for each driver.
 *
 *	This is nearly identical to the device iterator above.
 *	We iterate over each driver that belongs to @bus, and call
 *	@fn for each. If @fn returns anything but 0, we break out
 *	and return it. If @start is not NULL, we use it as the head
 *	of the list.
 *
 *	NOTE: we don't return the driver that returns a non-zero 
 *	value, nor do we leave the reference count incremented for that
 *	driver. If the caller needs to know that info, it must set it
 *	in the callback. It must also be sure to increment the refcount
 *	so it doesn't disappear before returning to the caller.
 */
/*
int bus_for_each_drv(struct bus_type * bus, struct device_driver * start,
		     void * data, int (*fn)(struct device_driver *, void *))
{
	struct list_head * head, * entry;
	int error = 0;

	if(!(bus = get_bus(bus)))
		return -EINVAL;

	head = start ? &start->kobj.entry : &bus->drivers.list;

	down_read(&bus->subsys.rwsem);
	list_for_each(entry,head) {
		struct device_driver * drv = get_driver(to_drv(entry));
		error = fn(drv,data);
		put_driver(drv);
		if(error)
			break;
	}
	up_read(&bus->subsys.rwsem);
	put_bus(bus);
	return error;
}
*/
/**
 *	device_bind_driver - bind a driver to one device.
 *	@dev:	device.
 *
 *	Allow manual attachment of a driver to a deivce.
 *	Caller must have already set @dev->driver.
 *
 *	Note that this does not modify the bus reference count 
 *	nor take the bus's rwsem. Please verify those are accounted 
 *	for before calling this. (It is ok to call with no other effort
 *	from a driver's probe() method.)
 */

void device_bind_driver(struct device * dev)
{
	pr_debug("bound device '%s' to driver '%s'\n",
		 dev->bus_id,dev->driver->name);
	list_add_tail(&dev->driver_list,&dev->driver->devices);
}


/**
 *	bus_match - check compatibility between device & driver.
 *	@dev:	device.
 *	@drv:	driver.
 *
 *	First, we call the bus's match function, which should compare
 *	the device IDs the driver supports with the device IDs of the 
 *	device. Note we don't do this ourselves because we don't know 
 *	the format of the ID structures, nor what is to be considered
 *	a match and what is not.
 *	
 *	If we find a match, we call @drv->probe(@dev) if it exists, and 
 *	call attach() above.
 */
/*
static int bus_match(struct device * dev, struct device_driver * drv)
{
	int error = -ENODEV;
	if (dev->bus->match(dev,drv)) {
		dev->driver = drv;
		if (drv->probe) {
			if ((error = drv->probe(dev))) {
				dev->driver = NULL;
				return error;
			}
		}
		device_bind_driver(dev);
		error = 0;
	}
	return error;
}
*/

/**
 *	device_attach - try to attach device to a driver.
 *	@dev:	device.
 *
 *	Walk the list of drivers that the bus has and call bus_match() 
 *	for each pair. If a compatible pair is found, break out and return.
 */
static int device_attach(struct device * dev)
{
 	struct bus_type * bus = dev->bus;
//	struct list_head * entry;
//	int error;

	if (dev->driver) {
		device_bind_driver(dev);
		return 1;
	}

	if (bus->match) {
printk("Fixme device_attach\n");
/*		list_for_each(entry,&bus->drivers.list) {
			struct device_driver * drv = to_drv(entry);
			error = bus_match(dev,drv);
			if (!error )  
*/				/* success, driver matched */
/*				return 1; 
			if (error != -ENODEV) 
*/				/* driver matched but the probe failed */
/*				printk(KERN_WARNING 
				    "%s: probe of %s failed with error %d\n",
				    drv->name, dev->bus_id, error);
		}
*/	}

	return 0;
}


/**
 *	driver_attach - try to bind driver to devices.
 *	@drv:	driver.
 *
 *	Walk the list of devices that the bus has on it and try to match
 *	the driver with each one.
 *	If bus_match() returns 0 and the @dev->driver is set, we've found
 *	a compatible pair.
 *
 *	Note that we ignore the -ENODEV error from bus_match(), since it's 
 *	perfectly valid for a driver not to bind to any devices.
 */
void driver_attach(struct device_driver * drv)
{
	struct bus_type * bus = drv->bus;
//	struct list_head * entry;
//	int error;

	if (!bus->match)
		return;
printk("Fixme driver_attach\n");
/*	list_for_each(entry,&bus->devices.list) {
		struct device * dev = container_of(entry,struct device,bus_list);
		if (!dev->driver) {
			error = bus_match(dev,drv);
			if (error && (error != -ENODEV))
*/				/* driver matched but the probe failed */
/*				printk(KERN_WARNING 
				    "%s: probe of %s failed with error %d\n",
				    drv->name, dev->bus_id, error);
		}
	}
*/}


/**
 *	device_release_driver - manually detach device from driver.
 *	@dev:	device.
 *
 *	Manually detach device from driver.
 *	Note that this is called without incrementing the bus
 *	reference count nor taking the bus's rwsem. Be sure that
 *	those are accounted for before calling this function.
 */

void device_release_driver(struct device * dev)
{
	struct device_driver * drv = dev->driver;
	if (drv) {
		list_del_init(&dev->driver_list);
		if (drv->remove)
			drv->remove(dev);
		dev->driver = NULL;
	}
}


/**
 *	driver_detach - detach driver from all devices it controls.
 *	@drv:	driver.
 */
static void driver_detach(struct device_driver * drv)
{
	struct list_head * entry, * next;
	list_for_each_safe(entry,next,&drv->devices) {
		struct device * dev = container_of(entry,struct device,driver_list);
		device_release_driver(dev);
	}
	
}

/**
 *	bus_add_device - add device to bus
 *	@dev:	device being added
 *
 *	- Add the device to its bus's list of devices.
 *	- Try to attach to driver.
 *	- Create link to device's physical location.
 */
int bus_add_device(struct device * dev)
{
	struct bus_type * bus = get_bus(dev->bus);
	int error = 0;

	if (bus) {
		down_write(&dev->bus->subsys.rwsem);
		pr_debug("bus %s: add device %s\n",bus->name,dev->bus_id);
		list_add_tail(&dev->bus_list,&dev->bus->devices.list);
		device_attach(dev);
		up_write(&dev->bus->subsys.rwsem);
	}
	return error;
}

/**
 *	bus_remove_device - remove device from bus
 *	@dev:	device to be removed
 *
 *	- Remove symlink from bus's directory.
 *	- Delete device from bus's list.
 *	- Detach from its driver.
 *	- Drop reference taken in bus_add_device().
 */
void bus_remove_device(struct device * dev)
{
	if (dev->bus) {
		down_write(&dev->bus->subsys.rwsem);
		pr_debug("bus %s: remove device %s\n",dev->bus->name,dev->bus_id);
		device_release_driver(dev);
		list_del_init(&dev->bus_list);
		up_write(&dev->bus->subsys.rwsem);
		put_bus(dev->bus);
	}
}


/**
 *	bus_add_driver - Add a driver to the bus.
 *	@drv:	driver.
 *
 */
int bus_add_driver(struct device_driver * drv)
{
	struct bus_type * bus = get_bus(drv->bus);
	int error = 0;

	if (bus) {
		pr_debug("bus %s: add driver %s\n",bus->name,drv->name);

		down_write(&bus->subsys.rwsem);
		driver_attach(drv);
		up_write(&bus->subsys.rwsem);

	}
	return error;
}


/**
 *	bus_remove_driver - delete driver from bus's knowledge.
 *	@drv:	driver.
 *
 *	Detach the driver from the devices it controls, and remove
 *	it from its bus's list of drivers. Finally, we drop the reference
 *	to the bus we took in bus_add_driver().
 */

void bus_remove_driver(struct device_driver * drv)
{
	if (drv->bus) {
		down_write(&drv->bus->subsys.rwsem);
		pr_debug("bus %s: remove driver %s\n",drv->bus->name,drv->name);
		driver_detach(drv);
		up_write(&drv->bus->subsys.rwsem);
		put_bus(drv->bus);
	}
}


/* Helper for bus_rescan_devices's iter */
/*
static int bus_rescan_devices_helper(struct device *dev, void *data)
{
	int *count = data;

	if (!dev->driver && device_attach(dev))
		(*count)++;

	return 0;
}
*/

struct bus_type * get_bus(struct bus_type * bus)
{
// Fixme: we should propably increment a reference count
	return bus;
}

void put_bus(struct bus_type * bus)
// Fixme: we should propably decrement a reference count
{
}

/**
 *	bus_register - register a bus with the system.
 *	@bus:	bus.
 *
 *	Once we have that, we registered the bus with the kobject
 *	infrastructure, then register the children subsystems it has:
 *	the devices and drivers that belong to the bus. 
 */
int bus_register(struct bus_type * bus)
{
// Fixme: various refcounts here	
	kobject_set_name(&bus->subsys.kset.kobj,bus->name);
	subsys_set_kset(bus,bus_subsys);
	init_rwsem(&bus->subsys.rwsem);
	INIT_LIST_HEAD(&bus->subsys.kset.list);
	atomic_set(&bus->subsys.kset.kobj.refcount,1);
	INIT_LIST_HEAD(&bus->subsys.kset.kobj.entry);
	if (!bus->subsys.kset.subsys) bus->subsys.kset.subsys = &bus->subsys;

	kobject_set_name(&bus->devices.kobj,"devices");
	bus->devices.subsys = &bus->subsys;
	atomic_set(&bus->devices.kobj.refcount,1);
	INIT_LIST_HEAD(&bus->devices.kobj.entry);
	INIT_LIST_HEAD(&bus->devices.list);
	
	kobject_set_name(&bus->drivers.kobj,"drivers");
	bus->drivers.subsys = &bus->subsys;
	atomic_set(&bus->drivers.kobj.refcount,1);
	INIT_LIST_HEAD(&bus->drivers.kobj.entry);
	INIT_LIST_HEAD(&bus->drivers.list);

	pr_debug("bus type '%s' registered\n",bus->name);
	return 0;
}


/**
 *	bus_unregister - remove a bus from the system 
 *	@bus:	bus.
 *
 *	Unregister the child subsystems and the bus itself.
 *	Finally, we call put_bus() to release the refcount
 */
void bus_unregister(struct bus_type * bus)
{
	pr_debug("bus %s: unregistering\n",bus->name);
}

int driver_register(struct device_driver * drv)
{
	INIT_LIST_HEAD(&drv->devices);
	init_MUTEX_LOCKED(&drv->unload_sem);
	return bus_add_driver(drv);
}

void driver_unregister(struct device_driver * drv)
{
	bus_remove_driver(drv);
	down(&drv->unload_sem);
	up(&drv->unload_sem);
}

int __init base_init(void)
{
	init_rwsem(&devices_subsys.rwsem);
	init_rwsem(&bus_subsys.rwsem);
	return 0;
}

module_init(base_init);
