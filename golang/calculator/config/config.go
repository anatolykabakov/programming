package config

import (
	"github.com/ilyakaznacheev/cleanenv"
	"log"
	"os"
)

type Config struct {
	StoragePath string `yaml:"storage_path" env-required:"true"`
	Address string `yaml:"address" env-default:"0.0.0.0:8080"`
}

func MustLoad() *Config {
	configPath := os.Getenv("CONFIG_PATH")
	if configPath == "" {
		log.Fatal("CONFIG_PATH is not set")
	}

	// check that config exists
	if _, err := os.Stat(configPath); err != nil {
		log.Fatalf("Config file not exists, ", err)
	}
	var config Config

	err := cleanenv.ReadConfig(configPath, &config)
	if err != nil {
		log.Fatalf("Error: ", err)
	}
	return &config
}
