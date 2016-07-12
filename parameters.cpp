#include <cstdio>
#include "parameters.h"
#include <cstdlib>
#include <unordered_map>
#include <vector>
#include <cstring>

// Program parameters

namespace vectorix {

void parameters::load_params(FILE *fd) { // Load parameters from file
	char *line;
	int linenumber = 0;

	while (fscanf(fd, "%m[^\n]\n", &line) >= 0) {
		linenumber++;
		if (!line) // Skip empty line
			continue;
		if (line[0] == '#') { // Skip commented line
			free(line);
			continue;
		}

		char *name;
		char *value;
		sscanf(line, "%m[^ ] %m[^\n]", &name, &value); // Split by first space
		if ((!name) && (!value)) {
			free(line);
			continue;
		}
		else if ((!name) && (value)) {
			free(value);
			free(line);
			continue;
		}
		else if ((name) && (!value)) {
			free(name);
			free(line);
			continue;
		}
		// Everything is ok

		std::string n = name;
		std::string v = value;

		int count_loaders = 0;
		for (auto &par: parameter_list) {
			if (n == par->name) {
				par->load_var(value);
				count_loaders++;
			}
		}
		if (!count_loaders) {
			auto old = not_loaded.find(n);
			if (old == not_loaded.end())
				not_loaded.insert({n,v});
			else {
				old->second = v;
			}
		}

		free(name);
		free(value);
		free(line);
	}
}

void parameters::save_params(FILE *fd) const { // Write parameters (with simple help) to opened filedescriptor
	for (auto const &par: parameter_list) {
		par->save_var(fd);
	}
}

void parameters::load_params(const std::string filename) { // Load parameters from file given by name
	FILE *fd;
	if ((fd = fopen(filename.c_str(), "r")) == NULL) {
		fprintf(stderr, "Couldn't open config file for reading.\n");
		return;
	}
	load_params(fd);
	fclose(fd);
}

void parameters::save_params(const std::string filename, bool append) const { // Save parameters to file given by name
	FILE *fd;
	if ((fd = fopen(filename.c_str(), (append) ? "a" : "w")) == NULL) {
		fprintf(stderr, "Couldn't open config file for writing.\n");
		return;
	}
	save_params(fd);
	fclose(fd);
}

void parameters::add_comment(const char *name) {
	std::shared_ptr<comment> s = std::make_shared<comment>();
	s->name = ((std::string) "# ") + name;

	auto old = binded_list.find(s->name);
	if (old == binded_list.end()) {
		parameter_list.push_back(s);
		binded_list.insert({s->name, s});
	}
}

}; // namespace
