terraform {
  required_version = ">=1.5.2"

  required_providers {
    azurerm = {
      source  = "hashicorp/azurerm"
      version = "~>3.63"
    }
    random = {
      source  = "hashicorp/random"
      version = "~>3.5"
    }
  }
}

provider "azurerm" {
  features {}
} 