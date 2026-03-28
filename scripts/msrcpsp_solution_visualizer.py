import pandas as pd
import matplotlib.pyplot as plt

solution_path = "./sol.sol"
instance_path = "./200_40_130_9_D4.def"

def read_instance_file(filename):
    with open(filename, 'r') as file:
        lines = file.readlines()

    general = {}
    resources = []
    tasks = []
    current_section = None

    for line in lines:
        line = line.strip()
        if not line:
            continue
        if set(line) == {"="}:
            continue

        if line.startswith("ResourceID"):
            current_section = "resources"
            continue
        if line.startswith("TaskID"):
            current_section = "tasks"
            continue

        if current_section == "resources":
            parts = line.split()
            if not parts:
                continue
            try:
                resource_id = int(parts[0])
            except ValueError:
                continue
            try:
                salary = float(parts[1])
            except ValueError:
                salary = None
            skills = {}
            i = 2
            while i < len(parts):
                skill = parts[i].replace(":", "")
                if i + 1 < len(parts):
                    try:
                        level = int(parts[i + 1])
                    except ValueError:
                        level = None
                    skills[skill] = level
                    i += 2
                else:
                    break
            resources.append({
                "ResourceID": resource_id,
                "Salary": salary,
                "Skills": skills
            })
            continue

        if current_section == "tasks":
            parts = line.split()
            if not parts:
                continue
            try:
                task_id = int(parts[0])
                duration = int(parts[1])
                skill_name = parts[2].replace(":", "")
                skill_level = int(parts[3])
                skill = f"{skill_name}::{skill_level}"
                predecessors = [int(x) for x in parts[4:]] if len(parts) > 4 else []
                tasks.append({
                    "TaskID": task_id,
                    "Duration": duration,
                    "Skill": skill,
                    "Predecessors": predecessors
                })
            except Exception:
                continue
            continue

    return general, pd.DataFrame(resources), pd.DataFrame(tasks)


def read_solution_file(filename):
    all_tasks = []
    with open(filename, 'r') as file:
        lines = file.readlines()

    for line in lines:
        tokens = line.strip().split()
        if not tokens:
            continue
        try:
            time = int(tokens[0])
            for token in tokens[1:]:
                resource_str, task_str = token.split('-')
                resource = int(resource_str)
                task = int(task_str)
                all_tasks.append({
                    'Time': time,
                    'Resource': resource,
                    'Task': task
                })
        except ValueError:
            continue

    return pd.DataFrame(all_tasks)


def validate_solution(tasks_df, solution_df, resources_df):
    errors = []
    task_durations = dict(zip(tasks_df['TaskID'], tasks_df['Duration']))
    task_predecessors = dict(zip(tasks_df['TaskID'], tasks_df['Predecessors']))
    resource_skills = dict(zip(resources_df['ResourceID'], resources_df['Skills']))

    task_end_times = {}
    resource_timeline = {}

    instance_tasks = set(tasks_df['TaskID'])
    for _, row in solution_df.iterrows():
        task_id = row['Task']
        if task_id not in instance_tasks:
            errors.append(f"Task {task_id} in solution does not exist in the instance.")

    for _, row in solution_df.iterrows():
        task_id = row['Task']
        start_time = row['Time']
        resource = row['Resource']
        duration = task_durations.get(task_id, 0)

        for t in range(start_time, start_time + duration):
            if resource not in resource_timeline:
                resource_timeline[resource] = set()
            if t in resource_timeline[resource]:
                errors.append(f"Resource {resource} is double-booked at time {t} for task {task_id}")
            resource_timeline[resource].add(t)

        task_end_times[task_id] = start_time + duration

        try:
            task_skill_full = tasks_df.loc[tasks_df['TaskID'] == task_id, 'Skill'].values[0]
            required_skill, req_level_str = task_skill_full.split("::")
            required_level = int(req_level_str)
        except Exception:
            errors.append(f"Could not parse skill requirement for task {task_id}")
            continue

        available_skills = resource_skills.get(resource, {})
        available_level = available_skills.get(required_skill, 0)
        if available_level < required_level:
            errors.append(
                f"Resource {resource} does not meet skill requirement for task {task_id} "
                f"(required {required_skill} level {required_level}, available level {available_level})"
            )

    for _, row in solution_df.iterrows():
        task_id = row['Task']
        start_time = row['Time']
        predecessors = task_predecessors.get(task_id, [])
        for pred in predecessors:
            pred_end_time = task_end_times.get(pred, None)
            if pred_end_time is None:
                errors.append(f"Predecessor task {pred} for task {task_id} is not scheduled.")
            elif pred_end_time > start_time:
                errors.append(
                    f"Task {task_id} starts at {start_time} before predecessor {pred} ends at {pred_end_time}"
                )

    task_counts = solution_df['Task'].value_counts()
    for task, count in task_counts.items():
        if count > 1:
            errors.append(f"Task {task} is scheduled {count} times.")
    missing_tasks = instance_tasks - set(solution_df['Task'])
    if missing_tasks:
        errors.append(f"Missing tasks: {missing_tasks}")

    return errors


def plot_schedule(tasks_df, solution_df):
    task_durations = dict(zip(tasks_df['TaskID'], tasks_df['Duration']))

    fig, ax = plt.subplots(figsize=(12, 8))

    for _, row in solution_df.iterrows():
        task_id = row['Task']
        resource = row['Resource']
        start_time = row['Time']
        duration = task_durations.get(task_id, 0)
        ax.barh(resource, duration, left=start_time, height=0.4, align='center', edgecolor='black')
        ax.text(start_time + duration / 2, resource, f"{task_id}", va='center', ha='center', color='white', fontsize=8)

    ax.set_xlabel("Time")
    ax.set_ylabel("Resource")
    ax.set_title("Gantt Chart of Schedule for 200_40_133_15")
    ax.set_yticks(sorted(solution_df['Resource'].unique()))
    plt.tight_layout()
    plt.show()


def main():
    general, resources_df, tasks_df = read_instance_file(instance_path)
    solution_df = read_solution_file(solution_path)

    errors = validate_solution(tasks_df, solution_df, resources_df)
    if errors:
        print("Validation Errors:")
        for err in errors:
            print("-", err)
    else:
        print("The solution is valid!")

    plot_schedule(tasks_df, solution_df)


if __name__ == "__main__":
    main()
