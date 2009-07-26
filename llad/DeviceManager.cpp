/*
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Library General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 * devicemanager.cpp
 * Implementation of the device manager, this object tracks what devices are in
 * use.
 * Copyright (C) 2005-2009 Simon Newton
 */

#include <stdio.h>
#include <errno.h>
#include <lla/Logging.h>
#include <lla/StringUtils.h>
#include <llad/Port.h>
#include "DeviceManager.h"

namespace lla {

const unsigned int DeviceManager::MISSING_DEVICE_ALIAS = 0;
const string DeviceManager::PORT_PREFERENCES = "port";

bool operator <(const device_alias_pair& left,
                const device_alias_pair &right) {
  if (left.alias < right.alias)
    return true;
  return false;
}


/*
 * Constructor
 */
DeviceManager::DeviceManager(PreferencesFactory *prefs_factory,
                             UniverseStore *universe_store):
    m_universe_store(universe_store),
    m_port_preferences(NULL),
    m_next_device_alias(FIRST_DEVICE_ALIAS) {

  if (prefs_factory) {
    m_port_preferences = prefs_factory->NewPreference(PORT_PREFERENCES);
    m_port_preferences->Load();
  }
}

/*
 * Cleanup
 */
DeviceManager::~DeviceManager() {
  if (m_port_preferences)
    m_port_preferences->Save();
}


/*
 * Register a device
 * @param device pointer to the device to register
 * @return true on success, false on failure
 */
bool DeviceManager::RegisterDevice(AbstractDevice *device) {
  if (!device)
    return false;

  string device_id = device->UniqueId();

  if (device_id.empty()) {
    LLA_WARN << "Device: " << device->Name() << " is missing UniqueId";
    return false;
  }

  unsigned int alias;
  map<string, device_alias_pair>::iterator iter = m_devices.find(device_id);
  if (iter != m_devices.end()) {
    if (iter->second.device) {
      // already registered
      LLA_INFO << "Device " << device_id << " is already registered";
      return false;
    } else {
      // was previously registered, reuse alias
      alias = iter->second.alias;
      iter->second.device = device;
    }
  } else {
    alias = m_next_device_alias++;
    device_alias_pair pair;
    pair.alias = alias;
    pair.device = device;
    m_devices[device_id] = pair;
  }

  m_alias_map[alias] = device;
  LLA_INFO << "Installed device: " << device->Name() << ":" <<
    device->UniqueId();

  RestoreDevicePortPatchings(device);
  return true;
}


/*
 * Unregister a device
 * @param device_id the id of the device to remove
 * @return true on sucess, false on failure
 */
bool DeviceManager::UnregisterDevice(const string &device_id) {
  map<string, device_alias_pair>::iterator iter =
    m_devices.find(device_id);

  if (iter == m_devices.end() || !iter->second.device) {
    LLA_WARN << "Device " << device_id << "not found";
    return false;
  }

  SaveDevicePortPatchings(iter->second.device);
  map<unsigned int, AbstractDevice*>::iterator alias_iter =
    m_alias_map.find(iter->second.alias);

  if (alias_iter != m_alias_map.end())
    m_alias_map.erase(alias_iter);

  iter->second.device = NULL;
  return true;
}

/*
 * Unregister a Device
 * @param device a pointer to the device
 * @return true on sucess, false on failure
 */
bool DeviceManager::UnregisterDevice(const AbstractDevice *device) {
  if (!device)
    return false;

  string device_id = device->UniqueId();
  if (device_id.empty())
    return false;

  return UnregisterDevice(device_id);
}


/*
 * Return the number of active devices
 * @return the number of active devices
 */
unsigned int DeviceManager::DeviceCount() const {
  unsigned int count = 0;
  map<string, device_alias_pair>::const_iterator iter;
  for (iter = m_devices.begin(); iter != m_devices.end(); ++iter)
    if (iter->second.device)
      count++;

  return count;
}


/*
 * Return a list of all the devices
 * @return a vector of device_alias_pairs
 */
vector<device_alias_pair> DeviceManager::Devices() const {
  vector<device_alias_pair> result;
  map<string, device_alias_pair>::const_iterator iter;
  for (iter = m_devices.begin(); iter != m_devices.end(); ++iter)
    if (iter->second.device)
      result.push_back(iter->second);

  return result;
}


/*
 * Find the device with the given alias.
 * @return a pointer to the device or NULL if the device wasn't found.
 */
AbstractDevice *DeviceManager::GetDevice(unsigned int alias) const {
  map<unsigned int, AbstractDevice*>::const_iterator alias_iter =
    m_alias_map.find(alias);
  if (alias_iter != m_alias_map.end())
    return alias_iter->second;
  return NULL;
}


/*
 * Return the device_alias_pair corresponding to the device with the given ID.
 * @param unique_id the unique id of the device
 * @return a device_alias_pair, if the device isn't found the alias is set to
 * MISSING_DEVICE_ALIAS and the device pointer is NULL.
 */
device_alias_pair DeviceManager::GetDevice(const string &unique_id) const {
  device_alias_pair result;
  map<string, device_alias_pair>::const_iterator iter =
    m_devices.find(unique_id);

  if (iter != m_devices.end() && iter->second.device)
      return iter->second;

  result.alias = MISSING_DEVICE_ALIAS;
  result.device = NULL;
  return result;
}


/*
 * Remove all devices and reset the device counter
 */
void DeviceManager::UnregisterAllDevices() {
  map<string, device_alias_pair>::iterator iter;
  for (iter = m_devices.begin(); iter != m_devices.end(); ++iter) {
    SaveDevicePortPatchings(iter->second.device);
    iter->second.device = NULL;
  }
  m_alias_map.clear();
}


/*
 * Save the port universe patchings for a device
 * @param device the device to save the settings for
 */
void DeviceManager::SaveDevicePortPatchings(AbstractDevice *device) {
  if (!m_port_preferences || !device)
    return;

  vector<AbstractPort*> ports = device->Ports();
  vector<AbstractPort*>::iterator port_iter;
  for (port_iter = ports.begin(); port_iter != ports.end(); ++port_iter) {
    if ((*port_iter)->UniqueId().empty())
      continue;

    if (!(*port_iter)->GetUniverse()) {
      m_port_preferences->RemoveValue((*port_iter)->UniqueId());
      continue;
    }

    m_port_preferences->SetValue(
        (*port_iter)->UniqueId(),
        IntToString((*port_iter)->GetUniverse()->UniverseId()));
  }
}

/*
 * Restore the port universe patchings for a device
 */
void DeviceManager::RestoreDevicePortPatchings(AbstractDevice *device) {
  if (!m_port_preferences || !device)
    return;

  vector<AbstractPort*> ports = device->Ports();
  for (vector<AbstractPort*>::iterator port_iter = ports.begin();
       port_iter != ports.end(); ++port_iter) {
    if ((*port_iter)->UniqueId().empty())
      continue;

    string uni_id = m_port_preferences->GetValue((*port_iter)->UniqueId());
    if (uni_id.empty())
      continue;

    errno = 0;
    int id = (int) strtol(uni_id.data(), NULL, 10);
    if (id == 0 && errno)
      continue;
    Universe *uni = m_universe_store->GetUniverseOrCreate(id);
    uni->AddPort(*port_iter);
  }
}

} //lla
