# Authentication Logger

The purpose of this docker container is to log all attempts of form input and to build an idea of what wordlists or injection attempts attackers may use.

## Autonomy 

This docker container spins up a python flask web application.

### To develop the application

1. Install python
   `winget install Python.Python.3.9`
2. I prefer to use envs to manage python depenendencies on a per app basis, these steps are optional, but are preferred to isolate and keep your dependencies clean and non-conflicting

`cd .\customer-containers\web-server-logger\` 
`py -m venv .venv` (I already created it within /customer-containers/web-server-logger)

`powershell.exe -executionpolicy unrestricted .\.venv\Scripts\activate`  (Temporily bypasses powershell script restricted to activate your python virtual env)


`python .\custom-containers\web-server-logger\username-password-recorder.py`