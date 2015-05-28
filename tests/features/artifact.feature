# language: en

Feature: Artifact
   In order to have business entities
   I have to be able to change an artifact


   Scenario Outline: Does a cool thing
      Given I have an artifact that does cool things
      When it <what>
      Then its events should reflect the cool things done

   Examples:
      | what                 |
      | wins the lottery     |
      | becomes President    |
      | executes time travel |
