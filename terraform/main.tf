resource "random_pet" "rg_name" {
  prefix = var.resource_group_name_prefix
}

resource "azurerm_resource_group" "rg" {
  location = var.resource_group_location
  name     = random_pet.rg_name.id
}

resource "azurerm_virtual_network" "vn" {
  name                = "internal-network"
  address_space       = ["10.0.0.0/24"]
  resource_group_name = azurerm_resource_group.rg.name
  location            = azurerm_resource_group.rg.location
}

resource "azurerm_subnet" "subnet" {
  name                 = "internal-subnet"
  resource_group_name  = azurerm_resource_group.rg.name
  virtual_network_name = azurerm_virtual_network.vn.name
  address_prefixes     = ["10.0.0.0/24"]
}

resource "azurerm_network_interface" "nic" {
  name                = "nic"
  location            = azurerm_resource_group.rg.location
  resource_group_name = azurerm_resource_group.rg.name
  ip_configuration {
    name                          = "internal"
    subnet_id                     = azurerm_subnet.subnet.id
    private_ip_address_allocation = "Dynamic"
    public_ip_address_id = azurerm_public_ip.public_ip.id
  }
}

resource "azurerm_linux_virtual_machine" "virtual-machine" {
  name                = "honey-badger-vm"
  resource_group_name = azurerm_resource_group.rg.name
  location            = azurerm_resource_group.rg.location

  size                = "Standard_B2as_v2" # We require at a minimum 8gb of memory. Take a gander here for Azure VM types (https://azure.microsoft.com/en-us/pricing/details/virtual-machines/linux/)
  admin_username      = "adminuser"
  custom_data = base64encode(templatefile("./template.tmpl", {"public_ip" = data.azurerm_public_ip.connect_ip.ip_address}))
  network_interface_ids = [azurerm_network_interface.nic.id,]

  admin_ssh_key {
    username   = "adminuser"
    public_key = file(var.ssh-key-path)
  }

# Should be demoted to a HDD if using permanently
  os_disk {
    caching              = "ReadWrite"
    storage_account_type = "Standard_LRS"
  }

  source_image_reference {
    publisher = "canonical"
    offer     = "0001-com-ubuntu-server-jammy"
    sku       = "22_04-lts-gen2"
    version   = "latest"
  }

 }

resource "azurerm_public_ip" "public_ip" {
  name                = "PublicIP"
  resource_group_name = azurerm_resource_group.rg.name
  location            = azurerm_resource_group.rg.location
  allocation_method   = "Static"
}


data "azurerm_public_ip" "connect_ip" {
  name = azurerm_public_ip.public_ip.name
  resource_group_name = azurerm_resource_group.rg.name
}

# resource "azurerm_network_interface_security_group_association" "association" {
#   network_interface_id      = azurerm_network_interface.rg.id
#   network_security_group_id = azurerm_network_security_group.rg.id
# }

# resource "azurerm_network_security_group" "nsg" {
#   name                = "ssh_nsg"
#   location            = azurerm_resource_group.rg.location
#   resource_group_name = azurerm_resource_group.rg.name

#   security_rule {
#     name                       = "allow_ssh_sg"
#     priority                   = 100
#     direction                  = "Inbound"
#     access                     = "Allow"
#     protocol                   = "Tcp"
#     source_port_range          = "*"
#     destination_port_range     = "22"
#     source_address_prefix      = "*"
#     destination_address_prefix = "*"
#   }
# }