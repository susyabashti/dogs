CREATE EXTENSION IF NOT EXISTS "pgcrypto";

CREATE TABLE IF NOT EXISTS dogs (
  id UUID PRIMARY KEY DEFAULT gen_random_uuid(),
  breed TEXT NOT NULL,
  country_origin TEXT NOT NULL,
  fur_color TEXT NOT NULL,
  eye_colors TEXT NOT NULL,
  min_height INT NOT NULL,
  max_height INT NOT NULL,
  min_longevity INT NOT NULL,
  max_longevity INT NOT NULL,
  character_traits TEXT NOT NULL,
  common_health_problems TEXT NOT NULL
);