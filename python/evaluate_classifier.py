import csv
from pathlib import Path

from ai.classifier.fashionclip_model import FashionCLIPModel
from ai.classifier.pipeline import ClothingPipeline

def read_correct_classifications_csv(csv_filename: str):
    print(f"Reading from file {csv_filename}\n")
    with open(csv_filename, "r", encoding="utf-8-sig", newline='') as csv_file:
        reader = csv.DictReader(csv_file, delimiter=',')
        headers = reader.fieldnames
        print(f"Column headers: {headers}\n")

        rows = {}
        for row in reader:
            rows[row[headers[0]]] = row
            print(f"{row[headers[0]]} : {rows[row[headers[0]]]}")

        return headers, rows


def run_classifier(csv_filename, image_names, headers):
    print("\n\nRunning classifier\n")

    pipeline = ClothingPipeline(FashionCLIPModel())
    results = {}
    num_errors = 0
    result_csv_filename = "classifier_results.csv"
    with open(result_csv_filename, "w", encoding="utf-8-sig", newline='') as csv_file:
        writer = csv.DictWriter(csv_file, fieldnames=headers + ["error"])
        writer.writeheader()
        for image_name in image_names:
            image_path = Path(f"./uploads/{image_name}")
            result = pipeline.analyze(image_path)

            if not result["ok"]:
                writer.writerow({"image": image_name, "error" : result["error"]})
                print(f"ERROR: {image_name} {result['error']}\n")
                num_errors += 1
                continue

            results[image_name] = {
                        "image": image_name,
                        "category": result["category"]["label"],
                        "sub_category": result["sub_category"]["label"],
                        "color": result["color"]["color"],
                        "formality": result["formality"]["formality"]
                    }
            writer.writerow(results[image_name])
            print(f"{image_name} : {results[image_name]}\n")

    return results, num_errors

def evaluate_results(csv_rows, classifier_results, csv_headers, image_names, num_errors):
    print("Evaluated results:")
    correct_numbers = {}
    successful_num = len(image_names)
    total_num = successful_num + num_errors

    print(f"Errors: {num_errors} / {total_num}\n")

    for header in csv_headers:
        if header == "image":
            continue
        incorrect_images = [i for i in image_names if csv_rows[i][header] != classifier_results[i][header]]
        correct_num = successful_num - len(incorrect_images)
        correct_numbers[header] = correct_num
        print(f"Number of correct {header}: {correct_num} / {successful_num}")
        print(f"Incorrect images: {','.join(incorrect_images)}\n")

    return correct_numbers

    

        
def main():
    csv_filename = "uploads_correct_classifications.csv"
    csv_headers, csv_rows = read_correct_classifications_csv(csv_filename)
    image_names = list(csv_rows.keys())
    classifier_results, num_errors = run_classifier(csv_filename, image_names, csv_headers)
    successful_images = list(classifier_results.keys())
    correct_numbers = evaluate_results(csv_rows, classifier_results, csv_headers, successful_images, num_errors)


if __name__ == "__main__":
    main()
