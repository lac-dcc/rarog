import json
import csv
import os

json_results_path = os.environ.get('JSON_RESULTS_PATH', '.')
csv_result_path = os.environ.get('CSV_RESULT_PATH', '.')

csv_data = []

models = ['alexnet', 'googlenet', 'inception_v3', 'mnasnet1_0', 'mobilenet_v2', 'resnet18', 'resnet34', 'resnet50', 'resnet101', 'resnet152', 'shufflenet', 'squeezenet']
# models = ['shufflenet', 'squeezenet']
metrics = ['mlir_lowering_time', 'total_compilation_time', 'execution_time', 'total_memory', 'buffer_memory']
methods = ['dynamic', 'static']

metrics_naming = {
    'mlir_lowering_time': 'MLIR Lowering Time (s)',
    'total_compilation_time': 'Compilation Time (s)',
    'execution_time': 'Running Time (s)',
    'total_memory': 'Max Memory Usage (mb)',
    'buffer_memory': 'Buffer memory usage (mb)'
}

row_idx = 0
for model_name in models:
    model_data = json.load(open(json_results_path+'/'+model_name+'.json','r'))

    csv_data.append([model_name, 'Dynamic', 'Static', 'Dynamic - Static', 'Static/Dynamic (%)'])
    for metric in metrics:
        csv_data.append([metrics_naming[metric], '', '', '', ''])

    r_idx = row_idx+1
    for metric in metrics:
        csv_data[r_idx][1] = str(model_data['dynamic'][metric]).replace('.',',')
        csv_data[r_idx][2] = str(model_data['static'][metric]).replace('.',',')
        csv_data[r_idx][3] =  str(round(model_data['dynamic'][metric]-model_data['static'][metric],2)).replace('.',',')
        csv_data[r_idx][4] =  str(round(model_data['static'][metric]/model_data['dynamic'][metric],4)).replace('.',',')
        r_idx += 1

    csv_data.append([])
    row_idx += 7

    

with open(csv_result_path+'/'+'mlir_bennu_data.csv', mode='w', newline='') as file:
    writer = csv.writer(file, delimiter=';', quoting=csv.QUOTE_MINIMAL)
    writer.writerows(csv_data)
