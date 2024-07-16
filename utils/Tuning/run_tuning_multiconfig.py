import os
import subprocess
import multiprocessing as mp

PROB_NAME = "ECVRPTW"
PROBLEM_DEF_ROOT = ""
OPTIMIZER_PATH = ""
ARCH_PATH_DIR = ""
CONFIGS_DEF_DIR = ""

EXECUTION_COUNT = 5
SEED = 0
CONF_BATCH = 10


def run_optimizer_for_file(file_path, conf_file, arch_dir):
    print('Begin optimization for file:', file_path)
    file_path = file_path.replace('\\', '/')
    short_filename = (file_path.rsplit('/', 1)[1]).split('.')[0]
    out_path = os.path.join(arch_dir, short_filename)
    process_output = subprocess.Popen([OPTIMIZER_PATH, conf_file, PROB_NAME, file_path,
                                       str(out_path), str(EXECUTION_COUNT), str(SEED)],
                                      stdin=subprocess.PIPE,
                                      stdout=subprocess.PIPE)
    result = process_output.communicate()[0].decode("utf-8")
    print(result)
    return result


def main():
    print("Number of processors: ", mp.cpu_count())
    for file in os.listdir(PROBLEM_DEF_ROOT):
        full_file_path = os.path.join(PROBLEM_DEF_ROOT, file)
        conf_l = os.listdir(CONFIGS_DEF_DIR)
        processes = []
        for i in range(0, len(conf_l), CONF_BATCH):
            print('Conf: ', i, '...')
            for conf in conf_l[i:min(len(conf_l), i+CONF_BATCH)]:
                conf_name = conf.split('.')[0]
                conf_file = os.path.join(CONFIGS_DEF_DIR, conf)
                arch_dir = os.path.join(ARCH_PATH_DIR, conf_name)
                if not os.path.exists(arch_dir):
                    os.mkdir(arch_dir)
                p = mp.Process(target=run_optimizer_for_file, args=(full_file_path, conf_file, arch_dir, ))
                p.start()
                processes.append(p)

            for p in processes:
                p.join()


if __name__ == '__main__':
    main()
