# language: en

Feature: Command
   In order to make changes to a system
   I have to be able to handle commands


   Scenario Outline: Simple command
      Given a message dispatcher
      When I send a dummy command message: "<message>"
      Then the last command should be: <string_value> <int_value>

   Examples:
      | message   | string_value | int_value |
      | hello 123 | hello        | 123       |
      | world 987 | world        | 987       |
