#ifndef __CAPMANAGER_H
#define __CAPMANAGER_H

#include <vector>
#include <string>
#include "capdescriptor.h"

/** Base class of capability managers, defines the capmanager interface. */
class CapManager
{
	protected:
		/*! \brief List of capability descriptors */
		std::vector<CapDescriptor *> _descriptors;

		/*!\brief Get cap descriptor for a certain task.
		 * \param  task              source task
		 * \return capdescriptor    if task is available
		 *         NULL             for unknown tasks
		 */
		CapDescriptor *getDescriptorForTask(unsigned int task);

		/*!\brief remove cap descriptor from the list
		 * \param  task             task to remove
		 * \return capdescriptor    removed object
		 *         NULL             no descriptor removed
		 */
		CapDescriptor *removeDescriptorForTask(unsigned int task);

	public:
		CapManager();
		virtual ~CapManager();

		/*!\brief Allow a task to communicate with a partner.
		 *
		 * \param src_task    source task
		 * \param dest_task   destination task
		 */
		virtual void allow(unsigned int src_task, unsigned int dest_task);

		/*!\brief Deny communication with a partner.
		 *
		 * \param src_task    source task
		 * \param dest_task   destination task
		 */
		virtual void deny(unsigned int src_task, unsigned int dest_task);

		/*!\brief Check whether interaction between to tasks is allowed.
		 *
		 * \param  src_task    source task
		 * \param  dest_task   destination task
		 * \return true        IPC allowed
		 * \return false       IPC denied
		 */
		virtual bool check(unsigned int src_task, unsigned int dest_task);
};

#endif
