import os

OUTPUT_PATH_ROOT = "C:/Users/adria/source/repos/iMOPSE_public/configurations/vec_output"
DEF_OPTION = "WTA_comp_30"
OUTPUT_ROOT_DIR = OUTPUT_PATH_ROOT

conf_f = open(os.path.join(OUTPUT_PATH_ROOT, DEF_OPTION.replace('/', '_')) + '.cfg', 'w')
total_path = os.path.join(OUTPUT_PATH_ROOT, DEF_OPTION).replace('\\', '/')
#lines = ["dir " + (os.path.join(total_path, conf_dir).replace('\\', '/') + ' ' + conf_dir + '\n') for conf_dir in os.listdir(total_path)]
lines = [(os.path.join(total_path, conf_dir).replace('\\', '/') + '\n') for conf_dir in os.listdir(total_path)]
lines[-1] = lines[-1][:-1]  # remove last new_line
conf_f.writelines(lines)
conf_f.close()

