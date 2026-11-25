# Copy files from source to destination while avoiding venv and pycache files.
file(COPY ${SRC}/ DESTINATION ${DST}
     PATTERN ".venv" EXCLUDE
     PATTERN "venv" EXCLUDE
     PATTERN "uploads" EXCLUDE # Yerin's 131 megabytes of clothes
     PATTERN "__pycache__" EXCLUDE)