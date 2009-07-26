/*
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * LlaDevice.h
 * Interface to the LLA Client Device class
 * Copyright (C) 2005-2006 Simon Newton
 */

#ifndef LLA_CLIENT_DEVICE_H
#define LLA_CLIENT_DEVICE_H

#include <string>
#include <vector>

namespace lla {

using std::string;
using std::vector;


/*
 * Represents a Plugin
 */
class LlaPlugin {
  public:
    LlaPlugin(unsigned int id, const string &name, const string &desc=""):
      m_id(id),
      m_name(name),
      m_description(desc) {};
    ~LlaPlugin() {};

    unsigned int Id() const { return m_id; }
    string Name() const { return m_name; }
    string Description() const { return m_description; }

    bool operator<(const LlaPlugin &other) const {
      return m_id < other.m_id;
    }
  private:
    unsigned int m_id; // id of this plugin
    string m_name;  // plugin name
    string m_description; // optional description
};


/*
 * Represents a port
 */
class LlaPort {
  public:
    enum PortCapability { LLA_PORT_CAP_IN, LLA_PORT_CAP_OUT};

    LlaPort(int port_id, PortCapability capability, int universe, int active,
            const string &description):
      m_id(port_id),
      m_capability(capability),
      m_universe(universe),
      m_active(active),
      m_description(description) {}
    ~LlaPort() {};

    int Id() const { return m_id; }
    PortCapability Capability() const { return m_capability; }
    int Universe() const { return m_universe; }
    int IsActive() const { return m_active; }
    string Description() const { return m_description; }

  private:
    int m_id; // id of this port
    PortCapability m_capability;  // port capability
    int m_universe; // universe
    int m_active; // active
    string m_description;
};


/*
 * Represents a device
 */
class LlaDevice {
  public:
    LlaDevice(const string &id,
              unsigned int alias,
              const string &name,
              int plugin_id,
              const vector<LlaPort> &ports):
      m_id(id),
      m_alias(alias),
      m_name(name),
      m_plugin_id(plugin_id),
      m_ports(ports) {}
    ~LlaDevice() {}

    string Id() const { return m_id; }
    unsigned int Alias() const { return m_alias; }
    string Name() const { return m_name; }
    int PluginId() const { return m_plugin_id; }

    const vector<LlaPort> Ports() const { return m_ports; }

    bool operator<(const LlaDevice &other) const {
      return m_alias < other.m_alias;
    }

  private:
    string m_id;            // device id
    unsigned int m_alias;   // device alias
    std::string m_name;  // device name
    int m_plugin_id;     // parent plugin id
    std::vector<LlaPort> m_ports;
};


/*
 * Represents a universe
 */
class LlaUniverse {
  public:
    enum merge_mode {
      MERGE_HTP,
      MERGE_LTP,
    };

    LlaUniverse(int id, merge_mode m, const string &name):
      m_id(id),
      m_merge_mode(m),
      m_name(name) {};
    ~LlaUniverse() {};

    int Id() const { return m_id;}
    merge_mode MergeMode() const { return m_merge_mode; }
    string Name() const { return m_name;}

  private:
    int m_id;      // id of this universe
    merge_mode m_merge_mode;  // merge mode
    string m_name;    // universe name

};

} //lla
#endif
