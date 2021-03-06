INTERFACE:

#include "mapping_tree.h"

class Mappable : public Base_mappable
{
public:
  virtual bool no_mappings() const;
  virtual ~Mappable() {}
};

IMPLEMENTATION:

IMPLEMENT
inline bool 
Mappable::no_mappings() const
{
  return !tree.get() || tree.get()->is_empty();
}
