import pandas as pd
import matplotlib.pyplot as plt


def read_and_format_data(filename):
    all_tasks = []
    project_info = {}
    instance_name = ""
    with open(filename, 'r') as file:
        lines = file.readlines()

    # Extract instance name and project information
    header_parts = lines[1].strip().split(';')
    instance_name = header_parts[0].split(': ')[1]
    project_info_keys = ['Duration', 'Cost', 'AvgCashFlowDev', 'AvgSkillOverUse', 'AvgUseOfResTime']
    project_info_values = header_parts[1:]
    project_info = dict(zip(project_info_keys, project_info_values))

    # Process tasks
    for line in lines[3:]:
        parts = line.strip().split(';')
        if len(parts) >= 2:
            try:
                time = int(parts[0])
            except ValueError:
                continue  # Skip non-numeric lines

            assignments = [a for a in parts[1:] if a]
            for assignment in assignments:
                task_parts = assignment.split('-')
                if len(task_parts) >= 4:
                    resource, task_id, duration, preds = task_parts
                    try:
                        resource, task_id, duration = map(int, [resource, task_id, duration])
                    except ValueError:
                        raise ValueError("Not all tasks are assigned to resources")
                    predecessors = [int(pred) for pred in preds.split(',') if
                                    pred]  # Convert each predecessor to an integer
                    all_tasks.append({
                        'Time': time,
                        'Resource': resource,
                        'Task': task_id,
                        'Duration': duration,
                        'Predecessors': predecessors
                    })

    tasks_df = pd.DataFrame(all_tasks)
    tasks_df = tasks_df.sort_values(by=['Resource', 'Time'])
    return tasks_df, project_info, instance_name


def validate_tasks(tasks_df):
    task_end_times = {}
    incorrect_order_reasons = {}  # Changed from a set to a dict to store reasons
    overlapping_tasks_reasons = {}  # Similarly, store reasons for overlaps

    # First pass to establish end times for all tasks
    for _, row in tasks_df.iterrows():
        end = row['Time'] + row['Duration']
        task_end_times[row['Task']] = end

    # Second pass for validation
    for _, row in tasks_df.iterrows():
        resource = row['Resource']
        start = row['Time']
        end = start + row['Duration']

        # Check against predecessors
        for predecessor in row['Predecessors']:
            pred_end = task_end_times.get(predecessor)
            if pred_end is not None and start < pred_end:
                reason = f"Starts before predecessor {predecessor} ends"
                incorrect_order_reasons[row['Task']] = reason

        # Check for overlapping tasks
        for _, other_row in tasks_df[tasks_df['Resource'] == resource].iterrows():
            if other_row['Task'] != row['Task']:  # Don't compare the task to itself
                other_start = other_row['Time']
                other_end = other_start + other_row['Duration']
                if start < other_end and end > other_start:
                    reason = f"Overlaps with task {other_row['Task']}"
                    overlapping_tasks_reasons[row['Task']] = reason
                    overlapping_tasks_reasons[other_row['Task']] = f"Overlaps with task {row['Task']}"

    return incorrect_order_reasons, overlapping_tasks_reasons


def plot_gantt_chart(tasks_df, project_info, instance_name, incorrect_order_reasons, overlapping_tasks_reasons):
    fig, ax = plt.subplots(figsize=(15, 10))
    for _, row in tasks_df.iterrows():
        task_id = row['Task']
        if task_id in incorrect_order_reasons:
            color = 'tab:red'
            reason = incorrect_order_reasons[task_id]
        elif task_id in overlapping_tasks_reasons:
            color = 'tab:orange'
            reason = overlapping_tasks_reasons[task_id]
        else:
            color = 'tab:blue'
            reason = ""

        # Draw the task bar
        rect = ax.barh(row['Resource'], row['Duration'], left=row['Time'], height=0.4, color=color, edgecolor='k')

        # Annotate the task ID and reason on the bar
        text_x = row['Time'] + row['Duration'] / 2  # Middle of the task duration
        text_y = row['Resource']
        annotation_text = f"{task_id}" + (f": {reason}" if reason else "")
        ax.text(text_x, text_y, annotation_text, ha='center', va='center', color='white', fontsize=8)

    # Adjusted error message for incorrect order and overlapping tasks
    error_messages = []
    if incorrect_order_reasons:
        error_messages.append("Incorrect task order detected!")
    if overlapping_tasks_reasons:
        error_messages.append("Overlapping tasks detected!")
    if not error_messages:  # If there are no errors
        error_messages.append("Solution is correct.")

    # Combine error messages and project info into one message
    error_message = ' | '.join(error_messages)
    project_info_text = ', '.join([f"{key}: {value}" for key, value in project_info.items()])
    plt.text(0.5, 1.10, f"{error_message} | {project_info_text}", ha='center', va='center', transform=ax.transAxes,
             color='black', fontsize=12, bbox=dict(facecolor='white', alpha=0.5))

    # Continue with the rest of the plot settings...
    ax.grid(True)
    ax.set_xlabel('Time')
    ax.set_ylabel('Resource')
    ax.set_title(f'Gantt Chart for {instance_name}: Resource Assignments with Validation')

    ax.set_yticks(tasks_df['Resource'].unique())
    ax.set_yticklabels([f'Resource {res}' for res in sorted(tasks_df['Resource'].unique())])

    plt.show()



# Main code
filename = '../../optimizer/experiments/MSRCPSP/run_0/best_solution.sol'
tasks_df, project_info, instance_name = read_and_format_data(filename)
incorrect_order_tasks, overlapping_tasks = validate_tasks(tasks_df)
plot_gantt_chart(tasks_df, project_info, instance_name, incorrect_order_tasks, overlapping_tasks)
