class SecretNumber:
  def __init__(init_val):
    self.secret_value = init_val

  def update_secret(my_val):
    return self.secret_value + my_val

secret = SecretNumber(7)
print("My secret number is " + str(secret.update_secret(10)) + "!")
