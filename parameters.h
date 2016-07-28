/*
 * Vectorix -- line-based image vectorizer
 * (c) 2016 Jan Hadrava <had@atrey.karlin.mff.cuni.cz>
 */
#ifndef VECTORIX__PARAMETERS_H
#define VECTORIX__PARAMETERS_H

// Program parameters

#include <string>
#include <cstdio>
#include <vector>
#include <memory>
#include <unordered_map>
#include "config.h"

namespace vectorix {

class parameters {
private:
	class param {
	public:
		virtual void save_var(FILE *fd) const = 0;
		virtual void load_var(const char *new_val) = 0;
		virtual void dafault_var() = 0;
		std::string name;
	protected:
		param() = default;
	};

	template <typename T> class param_spec: public param {};

	class comment: public param {
		virtual void save_var(FILE *fd) const {
			fprintf(fd, "%s\n", name.c_str());
		};
		virtual void load_var(const char *new_val) {};
		virtual void dafault_var() {};
	};
public:
	template <typename T> void bind_param(T *&variable, const char *name, const T&def_value) {
		std::shared_ptr<param_spec<T>> s = std::make_shared<param_spec<T>>();
		s->def_value = def_value;
		s->name = name;
		s->value = def_value;

		auto old = binded_list.find(s->name);
		if (old == binded_list.end()) {
			auto lazy_config = not_loaded.find(s->name);
			if (lazy_config != not_loaded.end()) {
				s->load_var(lazy_config->second.c_str());
			}

			parameter_list.push_back(s);
			binded_list.insert({s->name, s});
		}
		else {
			s = std::dynamic_pointer_cast<param_spec<T>> (old->second);
			if (!s) {
				fprintf(stderr, "Bug found: same parameter (%s) is used for two different types.\n", name);
				throw std::bad_cast();
			}
		}

		variable = &s->value;
	};
	void add_comment(const char *name);
	void reset_to_defaults();

	void load_params(FILE *fd); // Read from filedescriptor
	void load_params(const std::string &filename); // Load parameters from file given by name
	void save_params(FILE *fd) const; // Write to filedescriptor
	void save_params(const std::string &filename, bool append = true) const; // Save parameters to file given by name

	std::vector<std::shared_ptr<param>> parameter_list;
	std::unordered_map<std::string, std::string> not_loaded;
	std::unordered_map<std::string, std::shared_ptr<param>> binded_list;
};

template <>
class parameters::param_spec<int>: public param {
public:
	virtual void save_var(FILE *fd) const {
		fprintf(fd, "%s %i\n", name.c_str(), value);
	};
	virtual void load_var(const char *new_val) {
		sscanf(new_val, "%d", &value);
	};
	virtual void dafault_var() {
		value = def_value;
	};
	int value;
	int def_value;
};

template <>
class parameters::param_spec<float>: public param {
public:
	virtual void save_var(FILE *fd) const {
		fprintf(fd, "%s %f\n", name.c_str(), value);
	};
	virtual void load_var(const char *new_val) {
		sscanf(new_val, "%f", &value);
	};
	virtual void dafault_var() {
		value = def_value;
	};
	float value;
	float def_value;
};

template <>
class parameters::param_spec<double>: public param {
public:
	virtual void save_var(FILE *fd) const {
		fprintf(fd, "%s %f\n", name.c_str(), value);
	};
	virtual void load_var(const char *new_val) {
		sscanf(new_val, "%lf", &value);
	};
	virtual void dafault_var() {
		value = def_value;
	};
	double value;
	double def_value;
};

template <>
class parameters::param_spec<std::string>: public param {
public:
	virtual void save_var(FILE *fd) const {
		fprintf(fd, "%s %s\n", name.c_str(), value.c_str());
	};
	virtual void load_var(const char *new_val) {
		value = new_val;
	};
	virtual void dafault_var() {
		value = def_value;
	};
	std::string value;
	std::string def_value;
};

}; // namespace

#endif
