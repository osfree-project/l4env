/*!
 * \file   server/src/capdescriptor.h
 * \brief  Capability descriptor class
 *
 * \date   01/30/2007
 * \author doebel@os.inf.tu-dresden.de
 *
 */
/* (c) 2007 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details.
 */
#ifndef __SERVER_SRC_CAPDESCRIPTOR_H_
#define __SERVER_SRC_CAPDESCRIPTOR_H_
#include <vector>
#include <l4/log/l4log.h>

/*! \brief Descriptor maintaining a list of capabilities per task.
 */
class CapDescriptor
{
	private:
		/** task */
		unsigned int _task;
		/** _task's capabilities */
		std::vector<unsigned int> _caps;

	public:
		CapDescriptor(unsigned int task) : _task(task) {};
		~CapDescriptor() {};

		/*! \brief add capability
		 *
		 * Add a capability to communicate with task dest.
		 * \param dest     destination task
		 */
		void add_cap(unsigned int dest);

		/*! \brief remove capability
		 *
		 * Delete the capability to communicate with task dest.
		 * \param dest     destination task
		 */
		void remove_cap(unsigned int dest);

		/*! \brief Check right for task.
		 *
		 * Check if the descriptor contains a communication right
		 * for a task.
		 *
		 * \param task     destination task
		 * \return true    communication allowed
		 * \return false   communication denied
		 */
		bool has_cap(unsigned int task);
		
		/*! \brief Get number of capabilities
		 *
		 * \return >=0    number of capabilities
		 */
		unsigned inline num_caps(void) { return _caps.size(); }

		/*! \brief Get descriptor's task ID.
		 *
		 * \return >0     task ID this descriptor belongs to
		 */
		unsigned inline task(void) { return _task; }

		/*!\brief Dump capability list.
		 *
		 * Used for debugging purposes. Prints a list of all
		 * capabilities that are stored in this descriptor.
		 */
		void dumpCaps(void);
};
#endif
