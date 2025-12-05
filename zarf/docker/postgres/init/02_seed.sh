#!/bin/bash
set -e

echo "===> Loading initial dog data from CSV..."

psql -U "$POSTGRES_USER" -d "$POSTGRES_DB" -c "\
    COPY dogs(
        breed, 
        country_origin, 
        fur_color, 
        eye_colors, 
        character_traits, 
        common_health_problems, 
        min_height, 
        max_height, 
        min_longevity, 
        max_longevity
    )
    FROM '/docker-entrypoint-initdb.d/data.csv'
    DELIMITER ',' CSV HEADER;
"

echo "===> CSV import finished."
