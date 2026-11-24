
//
// @author hxAri (hxari)
// @create 2026-11-24 00:00
// @github https://github.com/uranite-lang/uranite
//
// Uranite Copyright (c) 2025 - hxAri <hxari@proton.me>
// Uranite Licence under GNU General Public Licence v3
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// any later version.
//
// You should have received a copy of the GNU General Public License
// along with this program. If not, see <https://www.gnu.org/licenses/>.
//

#include <gtest/gtest.h>
#include <memory>
#include <string>
#include <vector>

#include "uranite/ast/node.hpp"
#include "uranite/diagnostic/diagnostic.hpp"
#include "uranite/lexer/lexer.hpp"
#include "uranite/lookup/source.hpp"
#include "uranite/parser/parser.hpp"
#include "uranite/semantic/analyzer.hpp"
#include "uranite/semantic/symbol.hpp"
#include "uranite/token/token.hpp"

class FrontendFixesTest : public ::testing::Test {
	
	protected:
		
		uranite::diagnostic::Engine diagnostic { 100, 100 };
		
		uranite::ast::nodes::ProgramSharedPointer parse( const std::string& code ) {
			this->diagnostic.setConsolPrintable( false );
			uranite::lexer::Lexer lexer( code, "test.urn", this->diagnostic );
			uranite::parser::Parser parser( lexer.tokenize(), this->diagnostic );
			return parser.parse();
		}
		
		bool analyze( uranite::ast::nodes::ProgramSharedPointer program ) {
			this->diagnostic.setConsolPrintable( false );
			uranite::semantic::Analyzer analyzer( this->diagnostic );
			return analyzer.analyze( *program );
		}
		
		uranite::semantic::Analyzer analyzeWithAnalyzer( uranite::ast::nodes::ProgramSharedPointer program ) {
			this->diagnostic.setConsolPrintable( false );
			uranite::semantic::Analyzer analyzer( this->diagnostic );
			analyzer.analyze( *program );
			return analyzer;
		}
		
		uranite::ast::nodes::FunctionDeclaration* findFunction( uranite::ast::nodes::ProgramSharedPointer program, const std::string& name ) {
			for( uranite::ast::nodes::DeclarationSharedPointer& declaration : program->declarations ) {
				if( declaration != nullptr && declaration->kind == uranite::ast::Node::Kind::FunctionDeclaration ) {
					uranite::ast::nodes::FunctionDeclaration* functionDeclaration = static_cast<uranite::ast::nodes::FunctionDeclaration*>( declaration.get() );
					if( functionDeclaration->name == name ) {
						return functionDeclaration;
					}
				}
			}
			return nullptr;
		}
		
		uranite::ast::nodes::VariableStatement* findVariableInFunction( uranite::ast::nodes::FunctionDeclaration* functionDeclaration, const std::string& name ) {
			if( functionDeclaration == nullptr ) {
				return nullptr;
			}
			for( uranite::ast::nodes::StatementSharedPointer& statement : functionDeclaration->body ) {
				if( statement != nullptr && statement->kind == uranite::ast::Node::Kind::VariableStatement ) {
					uranite::ast::nodes::VariableStatement* variableStatement = static_cast<uranite::ast::nodes::VariableStatement*>( statement.get() );
					if( variableStatement->name == name ) {
						return variableStatement;
					}
				}
			}
			return nullptr;
		}
		
};

TEST_F( FrontendFixesTest, LocalVariableSourcePointsToName ) {
	uranite::ast::nodes::ProgramSharedPointer program( this->parse(
		"package testing\n"
		"\n"
		"public function main() -> Void:\n"
		"    I64 total = 0\n"
		"    total = 1\n"
	) );
	ASSERT_NE( program, nullptr );
	uranite::ast::nodes::FunctionDeclaration* functionDeclaration = this->findFunction( program, "main" );
	ASSERT_NE( functionDeclaration, nullptr );
	uranite::ast::nodes::VariableStatement* variableStatement = this->findVariableInFunction( functionDeclaration, "total" );
	ASSERT_NE( variableStatement, nullptr );
	ASSERT_NE( variableStatement->source, nullptr );
	ASSERT_NE( variableStatement->nameSource, nullptr );
	EXPECT_EQ( variableStatement->source->location->line, variableStatement->nameSource->location->line );
	EXPECT_EQ( variableStatement->source->location->column, variableStatement->nameSource->location->column );
	EXPECT_EQ( variableStatement->nameSource->location->line, 4u );
}

TEST_F( FrontendFixesTest, LocalVariableSymbolSourcePointsToName ) {
	uranite::ast::nodes::ProgramSharedPointer program( this->parse(
		"package testing\n"
		"\n"
		"public function main() -> Void:\n"
		"    I64 total = 0\n"
		"    total = 1\n"
	) );
	ASSERT_NE( program, nullptr );
	uranite::semantic::Analyzer analyzer( this->diagnostic );
	this->diagnostic.setConsolPrintable( false );
	analyzer.analyze( *program );
	uranite::ast::nodes::FunctionDeclaration* functionDeclaration = this->findFunction( program, "main" );
	ASSERT_NE( functionDeclaration, nullptr );
	// Find the identifier expression "total" in "total = 1" assignment
	uranite::ast::nodes::IdentifierExpression* totalIdentifier = nullptr;
	for( uranite::ast::nodes::StatementSharedPointer& statement : functionDeclaration->body ) {
		if( statement != nullptr && statement->kind == uranite::ast::Node::Kind::AssignmentStatement ) {
			uranite::ast::nodes::AssignStatement* assignStatement = static_cast<uranite::ast::nodes::AssignStatement*>( statement.get() );
			if( assignStatement->target != nullptr && assignStatement->target->kind == uranite::ast::Node::Kind::IdentifierExpression ) {
				totalIdentifier = static_cast<uranite::ast::nodes::IdentifierExpression*>( assignStatement->target.get() );
			}
		}
	}
	ASSERT_NE( totalIdentifier, nullptr );
	ASSERT_NE( totalIdentifier->resolvedSymbol, nullptr );
	ASSERT_NE( totalIdentifier->resolvedSymbol->source, nullptr );
	EXPECT_EQ( totalIdentifier->resolvedSymbol->source->location->line, 4u );
}

TEST_F( FrontendFixesTest, MemberAccessHasMemberSourceAndResolvedSymbol ) {
	uranite::ast::nodes::ProgramSharedPointer program( this->parse(
		"package testing\n"
		"\n"
		"public class User:\n"
		"    public String name\n"
		"    public function User( self, String name ) -> Void:\n"
		"        self.name = name\n"
		"\n"
		"public function main() -> Void:\n"
		"    User user = new User( \"alice\" )\n"
		"    String n = user.name\n"
	) );
	ASSERT_NE( program, nullptr );
	this->diagnostic.setConsolPrintable( false );
	uranite::semantic::Analyzer analyzer( this->diagnostic );
	analyzer.analyze( *program );
	uranite::ast::nodes::FunctionDeclaration* functionDeclaration = this->findFunction( program, "main" );
	ASSERT_NE( functionDeclaration, nullptr );
	uranite::ast::nodes::MemberAccessExpression* memberAccess = nullptr;
	for( uranite::ast::nodes::StatementSharedPointer& statement : functionDeclaration->body ) {
		if( statement != nullptr && statement->kind == uranite::ast::Node::Kind::VariableStatement ) {
			uranite::ast::nodes::VariableStatement* variableStatement = static_cast<uranite::ast::nodes::VariableStatement*>( statement.get() );
			if( variableStatement->name == "n" && variableStatement->initializer != nullptr &&
				variableStatement->initializer->kind == uranite::ast::Node::Kind::MemberAccessExpression ) {
				memberAccess = static_cast<uranite::ast::nodes::MemberAccessExpression*>( variableStatement->initializer.get() );
			}
		}
	}
	ASSERT_NE( memberAccess, nullptr );
	ASSERT_NE( memberAccess->memberSource, nullptr );
	EXPECT_EQ( memberAccess->member, "name" );
	EXPECT_NE( memberAccess->resolvedMemberSymbol, nullptr );
	EXPECT_EQ( memberAccess->resolvedMemberSymbol->name, "name" );
}

TEST_F( FrontendFixesTest, MethodCallHasMemberSourceAndResolvedSymbol ) {
	uranite::ast::nodes::ProgramSharedPointer program( this->parse(
		"package testing\n"
		"\n"
		"public class User:\n"
		"    public String name\n"
		"    public function User( self, String name ) -> Void:\n"
		"        self.name = name\n"
		"    public function getName( self ) -> String:\n"
		"        return self.name\n"
		"\n"
		"public function main() -> Void:\n"
		"    User user = new User( \"alice\" )\n"
		"    String n = user.getName()\n"
	) );
	ASSERT_NE( program, nullptr );
	this->diagnostic.setConsolPrintable( false );
	uranite::semantic::Analyzer analyzer( this->diagnostic );
	analyzer.analyze( *program );
	uranite::ast::nodes::FunctionDeclaration* functionDeclaration = this->findFunction( program, "main" );
	ASSERT_NE( functionDeclaration, nullptr );
	uranite::ast::nodes::MethodCallExpression* methodCall = nullptr;
	for( uranite::ast::nodes::StatementSharedPointer& statement : functionDeclaration->body ) {
		if( statement != nullptr && statement->kind == uranite::ast::Node::Kind::VariableStatement ) {
			uranite::ast::nodes::VariableStatement* variableStatement = static_cast<uranite::ast::nodes::VariableStatement*>( statement.get() );
			if( variableStatement->name == "n" && variableStatement->initializer != nullptr &&
				variableStatement->initializer->kind == uranite::ast::Node::Kind::MethodCallExpression ) {
				methodCall = static_cast<uranite::ast::nodes::MethodCallExpression*>( variableStatement->initializer.get() );
			}
		}
	}
	ASSERT_NE( methodCall, nullptr );
	ASSERT_NE( methodCall->memberSource, nullptr );
	EXPECT_EQ( methodCall->method, "getName" );
	EXPECT_NE( methodCall->resolvedMemberSymbol, nullptr );
	EXPECT_EQ( methodCall->resolvedMemberSymbol->name, "getName" );
}

TEST_F( FrontendFixesTest, ChainedCallFieldAccessParsesAndResolves ) {
	uranite::ast::nodes::ProgramSharedPointer program( this->parse(
		"package testing\n"
		"\n"
		"public class User:\n"
		"    public String name\n"
		"    public function User( self, String name ) -> Void:\n"
		"        self.name = name\n"
		"    public function getName( self ) -> String:\n"
		"        return self.name\n"
		"\n"
		"public function getUser() -> User:\n"
		"    return new User( \"alice\" )\n"
		"\n"
		"public function main() -> Void:\n"
		"    String a = getUser().name\n"
		"    String b = getUser().getName()\n"
	) );
	ASSERT_NE( program, nullptr );
	this->diagnostic.setConsolPrintable( false );
	uranite::semantic::Analyzer analyzer( this->diagnostic );
	bool ok = analyzer.analyze( *program );
	EXPECT_TRUE( ok );
}

TEST_F( FrontendFixesTest, DeferBlockResolvesSymbols ) {
	uranite::ast::nodes::ProgramSharedPointer program( this->parse(
		"package testing\n"
		"\n"
		"public function main() -> Void:\n"
		"    I64 x = 1\n"
		"    defer:\n"
		"        x = x + 1\n"
	) );
	ASSERT_NE( program, nullptr );
	this->diagnostic.setConsolPrintable( false );
	uranite::semantic::Analyzer analyzer( this->diagnostic );
	bool ok = analyzer.analyze( *program );
	EXPECT_TRUE( ok );
	uranite::ast::nodes::FunctionDeclaration* functionDeclaration = this->findFunction( program, "main" );
	ASSERT_NE( functionDeclaration, nullptr );
	uranite::ast::nodes::DeferStatement* deferStatement = nullptr;
	for( uranite::ast::nodes::StatementSharedPointer& statement : functionDeclaration->body ) {
		if( statement != nullptr && statement->kind == uranite::ast::Node::Kind::DeferStatement ) {
			deferStatement = static_cast<uranite::ast::nodes::DeferStatement*>( statement.get() );
		}
	}
	ASSERT_NE( deferStatement, nullptr );
	ASSERT_NE( deferStatement->body, nullptr );
	if( deferStatement->body->kind == uranite::ast::Node::Kind::BlockStatement ) {
		uranite::ast::nodes::BlockStatement* block = static_cast<uranite::ast::nodes::BlockStatement*>( deferStatement->body.get() );
		ASSERT_FALSE( block->statements.empty() );
		uranite::ast::nodes::StatementSharedPointer& innerStmt = block->statements[0];
		ASSERT_NE( innerStmt, nullptr );
		if( innerStmt->kind == uranite::ast::Node::Kind::AssignmentStatement ) {
			uranite::ast::nodes::AssignStatement* assign = static_cast<uranite::ast::nodes::AssignStatement*>( innerStmt.get() );
			ASSERT_NE( assign->target, nullptr );
			if( assign->target->kind == uranite::ast::Node::Kind::IdentifierExpression ) {
				uranite::ast::nodes::IdentifierExpression* ident = static_cast<uranite::ast::nodes::IdentifierExpression*>( assign->target.get() );
				EXPECT_NE( ident->resolvedSymbol, nullptr );
			}
		}
	}
}

TEST_F( FrontendFixesTest, CompletionOnMemberAccess ) {
	uranite::ast::nodes::ProgramSharedPointer program( this->parse(
		"package testing\n"
		"\n"
		"public class User:\n"
		"    public String name\n"
		"    public function User( self, String name ) -> Void:\n"
		"        self.name = name\n"
		"    public function getName( self ) -> String:\n"
		"        return self.name\n"
		"\n"
		"public function main() -> Void:\n"
		"    User user = new User( \"alice\" )\n"
		"    user.\n"
	) );
	ASSERT_NE( program, nullptr );
	// set completion position at "user." on line 12, after the dot
	uranite::lookup::SourceSharedPointer completionSource = std::make_shared<uranite::lookup::Source>();
	completionSource->location->line = 12;
	completionSource->location->column = 10;
	this->diagnostic.setConsolPrintable( false );
	uranite::semantic::Analyzer analyzer( this->diagnostic );
	analyzer.setCompletionPosition( completionSource );
	analyzer.analyze( *program );
	bool hasName = false;
	bool hasGetName = false;
	for( const uranite::semantic::CompletionItem& item : analyzer.completions() ) {
		if( item.label == "name" ) hasName = true;
		if( item.label == "getName" ) hasGetName = true;
	}
	EXPECT_TRUE( hasName );
	EXPECT_TRUE( hasGetName );
}

TEST_F( FrontendFixesTest, CompletionOnSelfMemberAccess ) {
	uranite::ast::nodes::ProgramSharedPointer program( this->parse(
		"package testing\n"
		"\n"
		"public class User:\n"
		"    public String name\n"
		"    public function User( self, String name ) -> Void:\n"
		"        self.\n"
	) );
	ASSERT_NE( program, nullptr );
	uranite::lookup::SourceSharedPointer completionSource = std::make_shared<uranite::lookup::Source>();
	completionSource->location->line = 6;
	completionSource->location->column = 14;
	this->diagnostic.setConsolPrintable( false );
	uranite::semantic::Analyzer analyzer( this->diagnostic );
	analyzer.setCompletionPosition( completionSource );
	analyzer.analyze( *program );
	bool hasName = false;
	for( const uranite::semantic::CompletionItem& item : analyzer.completions() ) {
		if( item.label == "name" ) hasName = true;
	}
	EXPECT_TRUE( hasName );
}

TEST_F( FrontendFixesTest, CompletionOnParentMemberAccess ) {
	uranite::ast::nodes::ProgramSharedPointer program( this->parse(
		"package testing\n"
		"\n"
		"public class Base:\n"
		"    public String name\n"
		"    public function Base( self, String name ) -> Void:\n"
		"        self.name = name\n"
		"    public function getName( self ) -> String:\n"
		"        return self.name\n"
		"\n"
		"public class Child extends Base:\n"
		"    public function Child( self, String name ) -> Void:\n"
		"        parent( name )\n"
		"    public function who( self ) -> String:\n"
		"        return parent.\n"
	) );
	ASSERT_NE( program, nullptr );
	uranite::lookup::SourceSharedPointer completionSource = std::make_shared<uranite::lookup::Source>();
	completionSource->location->line = 14;
	completionSource->location->column = 23;
	this->diagnostic.setConsolPrintable( false );
	uranite::semantic::Analyzer analyzer( this->diagnostic );
	analyzer.setCompletionPosition( completionSource );
	analyzer.analyze( *program );
	bool hasGetName = false;
	bool hasName = false;
	std::string labels;
	for( const uranite::semantic::CompletionItem& item : analyzer.completions() ) {
		labels += item.label + ",";
		if( item.label == "getName" ) hasGetName = true;
		if( item.label == "name" ) hasName = true;
	}
	EXPECT_TRUE( hasGetName || hasName ) << labels;
}

TEST_F( FrontendFixesTest, CompletionOnGenericTypeArgument ) {
	uranite::ast::nodes::ProgramSharedPointer program( this->parse(
		"package testing\n"
		"\n"
		"public class Box<T>:\n"
		"    public T value\n"
		"\n"
		"public function main() -> Void:\n"
		"    Box<String> box\n"
	) );
	ASSERT_NE( program, nullptr );
	uranite::lookup::SourceSharedPointer completionSource = std::make_shared<uranite::lookup::Source>();
	completionSource->location->line = 7;
	completionSource->location->column = 10;
	this->diagnostic.setConsolPrintable( false );
	uranite::semantic::Analyzer analyzer( this->diagnostic );
	analyzer.setCompletionPosition( completionSource );
	analyzer.analyze( *program );
	EXPECT_FALSE( analyzer.completions().empty() );
}
