import os
import copy


def generate_configs_for_method(template, configs_path):
    if not os.path.exists(configs_path):
        os.makedirs(configs_path)
    all_configs = []
    build_configs_recursive(all_configs, template, 0, {})
    for i in range(len(all_configs)):
        conf = all_configs[i]
        print(conf)
        filename = 'config_' + str(i)
        f = open(os.path.join(configs_path, filename) + '.cfg', 'w')
        lines = [(key + ' ' + value + '\n') for (key, value) in conf.items()]
        lines[-1] = lines[-1][:-1]  # remove last new_line
        f.writelines(lines)
        f.close()


def build_configs_recursive(all_configs, config_template, param_idx, config_instance):
    template_keys = list(config_template.keys())
    param_name = template_keys[param_idx]
    for param_option in config_template[param_name]:
        config_instance[param_name] = str(param_option)
        if param_idx + 1 < len(template_keys):
            build_configs_recursive(all_configs, config_template, param_idx + 1, config_instance)
        else:
            all_configs.append(copy.deepcopy(config_instance))
