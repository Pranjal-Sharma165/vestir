# Copy files from source to destination while avoiding venv and pycache files.
file(COPY ${SRC}/ DESTINATION ${DST}
     PATTERN ".venv" EXCLUDE
     PATTERN "venv" EXCLUDE
     PATTERN "__pycache__" EXCLUDE)