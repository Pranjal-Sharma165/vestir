from ai.recommendation.scoring import calculate_formality_score
from ai.recommendation.scoring import calculate_rain_score
from ai.recommendation.scoring import calculate_outfit_score
outfit = {
    "items": [
        {
            "category_name": "Top",
            "formality": "casual",
            "primary_color": "black"
        },
        {
            "category_name": "Bottom",
            "formality": "smart-casual",
            "primary_color": "blue"
        },
        {
            "category_name": "Outerwear",
            "formality": "formal",
            "primary_color": "white"
        }
    ]
}

score = calculate_formality_score(outfit)

print("Formality score:", score)
print(calculate_rain_score(outfit, 10))
print(calculate_rain_score(outfit, 60))
print(calculate_rain_score(outfit, 90))

score = calculate_outfit_score(
    outfit,
    temperature=10,
    rain_probability=60,
    event="school"
)

print("Overall outfit score:", score)

