#ifndef __L4_EXEC_SERVER_DSC_H_
#define __L4_EXEC_SERVER_DSC_H_

#include <l4/sys/types.h>

class dsc_obj_t;

/** Describes an array of objects identified by an unique identifier. 
 * Objects of array can be found by lookup() and find() */
class dsc_array_t 
{
  public:
    dsc_array_t(l4_uint32_t n);

    int init(void);
    int alloc(dsc_obj_t ***dsc_obj, l4_uint32_t *id);
    int free(l4_uint32_t id);
    dsc_obj_t *lookup(l4_uint32_t id);
    dsc_obj_t *find(const char *pathname);

  protected:
    l4_uint32_t	get_next_unique_id(void);

    l4_uint32_t	entries;
    dsc_obj_t	**dsc_objs;
    dsc_obj_t	**dsc_free_obj;

  private:
    l4_uint32_t	unique_id;
};

/** Describes an object which is identified by an unique identifier */
class dsc_obj_t
{
  public:
    inline dsc_obj_t(dsc_array_t *_dsc_array, l4_uint32_t _id)
      : id(_id), dsc_array(_dsc_array)
      {}
    virtual inline ~dsc_obj_t()
      { dsc_array->free(id); }
    
    inline l4_uint32_t get_id(void)
      { return id; }

    virtual inline const char *get_pathname(void)
      { return (const char*)NULL; }

  private:
    dsc_obj_t();
    
    l4_uint32_t	id;
    dsc_array_t	*dsc_array;
};

#define UNIQUE_ID_ADD	0x00010000
#define UNIQUE_ID_MASK	0x0000ffff

#endif

