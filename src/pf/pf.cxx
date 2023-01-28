#include <fcntl.h>

#include "dsLog.h"
#include "pf.h"

using namespace libdms5;


bool PFTable::_enabled = false;
int PFTable::_fd = -1;
const char *PFTable::_device = NULL;
const char *PFTable::_default_table = NULL;

// factory
void PFTable::initialize(const char *device, const char *table_name) {
  _default_table = table_name;
  _device = device;

  if ((_fd = open(_device, O_RDWR)) < 0) {
    throw PFException("Not able to open pf device %s (%m)", _device);
  }

  dsLog::info("PF support enabled on %s '%s'", _device, _default_table);
  _enabled = true;
}
