'use strict';

const Generator = require('yeoman-generator');
const chalk = require('chalk');
const yosay = require('yosay');

module.exports = class extends Generator {
  async prompting() {
    this.log(
      yosay(`Welcome to the ${chalk.red('MPL')} project generator!`)
    );

    this.answers = await this.prompt([
      {
        type: 'input',
        name: 'projectName',
        message: 'Project name:',
        default: this.appname.replace(/\s+/g, '-'),
      },
      {
        type: 'input',
        name: 'author',
        message: 'Author:',
        default: this.user.git.name() || '',
      },
      {
        type: 'input',
        name: 'compiler',
        message: 'Path to the MPL compiler binary (comp):',
        default: 'comp',
      },
      {
        type: 'confirm',
        name: 'examples',
        message: 'Include example programs?',
        default: true,
      },
    ]);
  }

  writing() {
    const ctx = {
      projectName: this.answers.projectName,
      author: this.answers.author,
      compiler: this.answers.compiler,
    };

    // Core source file
    this.fs.copyTpl(
      this.templatePath('main.mpl'),
      this.destinationPath('src/main.mpl'),
      ctx
    );

    // Build + project files (EJS-templated)
    this.fs.copyTpl(
      this.templatePath('Makefile'),
      this.destinationPath('Makefile'),
      ctx
    );
    this.fs.copyTpl(
      this.templatePath('README.md'),
      this.destinationPath('README.md'),
      ctx
    );

    // Verbatim files (gitignore is renamed so npm doesn't strip it on publish)
    this.fs.copy(
      this.templatePath('gitignore'),
      this.destinationPath('.gitignore')
    );

    if (this.answers.examples) {
      this.fs.copy(
        this.templatePath('examples'),
        this.destinationPath('examples')
      );
    }
  }

  end() {
    this.log('\n' + chalk.green('Done!') + ' Your MPL project is ready.\n');
    this.log('Next:');
    this.log(chalk.cyan(`  cd ${this.answers.projectName}`));
    this.log(chalk.cyan('  make          ') + '# build src/main.mpl -> bin/main');
    this.log(chalk.cyan('  make run      ') + '# build and run\n');
  }
};
