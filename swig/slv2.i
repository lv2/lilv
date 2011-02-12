%module slv2
%{
#include "slv2/slv2.h"
#include "slv2/slv2mm.hpp"
%}

%include "slv2/slv2.h"
%include "slv2/slv2mm.hpp"

namespace SLV2 {

%extend Plugins {
%pythoncode %{
	def __iter__(self):
		class Iterator(object):
			def __init__(self, plugins):
				self.plugins = plugins
				self.index   = 0
	                
			def __iter__(self):
				return self
		
			def next(self):
				self.index += 1
				if self.index < slv2_plugins_size(self.plugins.me):
					return Plugin(slv2_plugins_get_at(self.plugins.me, self.index))
				else:
					raise StopIteration

		return Iterator(self)
%}
};

%extend Value {
%pythoncode %{
	def __str__(self):
		return slv2_value_get_turtle_token(self.me)
%}
};

%extend World {
%pythoncode %{
	def get_plugin(self, uri_str):
		return Plugin(slv2_world_get_plugin_by_uri_string(self.me, uri_str))
%}
};

} /* namespace SLV2 */
