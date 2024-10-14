output "resource_group_name" {
  value = azurerm_resource_group.rg.name
}

output "connect_ip" {
  value = data.azurerm_public_ip.connect_ip.ip_address
} # Outputs the IP of the VM

# output "template" {
#   value = data.template_file.cloud-init
# }