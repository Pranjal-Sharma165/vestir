import time

from clothing_classifier import ClothingClassifier

# Measure model initialization
start = time.perf_counter()
classifier = ClothingClassifier()
load_time = time.perf_counter() - start

print(f"Model loading time: {load_time:.2f} seconds")

# Measure prediction time
image_path = "ai/test_image.jpeg"

start = time.perf_counter()
result = classifier.predict(image_path)
prediction_time = time.perf_counter() - start

print("Predicted category:", result["category"])
print("Confidence score:", round(result["confidence"], 3))
print("All scores:", result["scores"])
print(f"Prediction time: {prediction_time:.2f} seconds")
print("Device:", classifier.device)